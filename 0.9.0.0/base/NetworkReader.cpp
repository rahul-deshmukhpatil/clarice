#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/select.h>
#include <stdlib.h>
#include <unordered_map>
#include <boost/tokenizer.hpp>

#include "infra/logger/Logger.h"
#include "infra/utils/StringUtils.h"
#include "base/NetworkReader.h"
#include "base/MarketDataApplication.h"
#include "base/FeedHandler.h"
#include "base/LineGroup.h"
#include "base/Line.h"
#include "base/Packet.h"
#include "base/Recorder.h"

using namespace base;
using namespace infra;

/**
 *	\brief:	NetworkReader
 *			Constructor creates the Network reader object and thread belonging to it.
 */

NetworkReader::NetworkReader(const MarketDataApplication *app, const pugi::xml_node &readerNode)
	: _appInstance(app)
	, _name()
	, _stopApp(false)
	, _updateIPVector(false)
	, _mode(PlaybackMode::LIVE_PLAYBACK)
	, _recordQueue(nullptr)
{
	strncpy(const_cast<char *>(_name), readerNode.getAttributeAsCString("name", "", false, ""), sizeof(_name));
    logConsole(INFO, "Created the NetworkReader [ %s : %p ]", _name, this);    
	_thread = new Thread(_appInstance, readerNode, _name, this, NetworkReader::start);	
}

/*
 * Register LineGroups files
 *	if 
 */
void NetworkReader::registerLineGroup(LineGroup *lg)
{
	// Ensure that we have same playback mode for all the feeds under this network reader
	if(_feedStreams.size() != 0)
	{
		if(_mode != lg->_feedHandler->_mode)
		{
			///< Throw an error exception duplicate reader name 
			char errorBuff[512];

			snprintf(errorBuff, sizeof(errorBuff), "For NetworkReader [ %s : %p ] : LineGroup [ %s , %p ] is being registered. But FeedHandler [ %s : %p ] has playback mode [ %s ] different than perviosuly registered feeds for this NetworkReader [%s]", _name, this, lg->_name, lg, lg->_feedHandler->_name, lg->_feedHandler, playbackModeToStr(lg->_feedHandler->_mode), playbackModeToStr(_mode));
			std::string error(errorBuff);
			throw std::invalid_argument(error);
		}
	}
	
	_mode = lg->_feedHandler->_mode;
	if(lg->_feedHandler->_recording)
	{
		_recordQueue = _appInstance->getRecorder()->registerNetworkReader(this);	
	}
	
	// In FILE_PLAYBACK mode write the recordings into the _feedHandler map

	if(_mode == PlaybackMode::FILE_PLAYBACK)
	{
		const std::vector<std::string> &recordings = lg->_feedHandler->_playbackFilesVector;
		std::vector<Recording *>& streams = _feedStreams[lg->_feedHandler];

		//do not add same _feedHandler recordings twice for multiple lineGroups registrations
		if(streams.empty())
		{
			for(const std::string &recording : recordings)	
			{
				//open recording for reading 
				streams.push_back(new Recording(lg->_feedHandler, recording, false));
			}
		}
	}
}

/**
 * Starts the actual OS thread instance
 */
void NetworkReader::start()
{
	_thread->start();
	return;
}

/**
 * Stops actual OS thread instance
 */
void NetworkReader::stop()
{
	_thread->signalToStop();

	while(!_thread->isStopped());

	logConsole(INFO, "NetworkReader [ %s : %p ] is stopped", _name, this);
	return;
}

/**
 * Calls the processing function
 */
void* NetworkReader::start(void *obj)
{
	NetworkReader *nr = static_cast<NetworkReader *>(obj);
	if(nr->_mode == PlaybackMode::LIVE_PLAYBACK)
	{
		nr->networkReaderLoop();
	}
	else
	{
		nr->networkReaderLoopFile();
	}
	return nullptr;
}

/**
 * \brief: addIPToNetworkReader
 *			
 *
 */
void NetworkReader::addIPToNetworkReader(const PacketAddress* pPacketAddress)
{
	WLock wlock(_mutexToAddRemove);
	///@TODO: create socket here 
	_addIPs.push_back(pPacketAddress);
	_updateIPVector = true;
}

/**
 * \brief: removeIPFromNetworkReader
 *			
 *
 */
 ///@TODO: move this function to the Line. As someone might try to log something here
void NetworkReader::removeIPFromNetworkReader(const PacketAddress* pPacketAddress)
{
	WLock wlock(_mutexToAddRemove);
	///@TODO: create socket here 
	_removeIPs.push_back(pPacketAddress);
	_updateIPVector = true;
}

/**
 * \brief: networkReaderLoop 	
 * 			Read all the active IPs 	
 */
void NetworkReader::networkReaderLoop()
{
	//Read ech of the packet from all the registered lines
	logMessage(INFO, "Starting a Network Loop for NetworkReader [ %s : %p ] in LIVE_PLAYBACK mode", _name, this);

	std::unordered_map<int32_t, const PacketAddress *> socketfdMap;
	fd_set rfds;
	struct timespec ts = {.tv_sec = 1, .tv_nsec = 1000};
	struct timespec tv = {.tv_sec = 1, .tv_nsec = 1000};
	int32_t nfds = -1;

	while(_thread->isActive())
	{
		if(_updateIPVector)
		{
		 	//Add and remove the sockets that are modified
			updateIPVector();

			FD_ZERO(&rfds);
			nfds = -1;
			for(auto itr: _ipReadVector)
			{
				int32_t fd = itr->socket().socketfd();
				FD_SET(fd, &rfds);
				if(fd > nfds)
				{
					nfds = fd;
				}
			}
			nfds = nfds + 1;
		}

		fd_set temprfds = rfds;
		int retval = pselect(nfds, &temprfds, NULL, NULL, &ts, NULL);
		//int retval = select(nfds, &temprfds, NULL, NULL, &tv);
		
		if(retval > 0)
		{
			for(auto itr: _ipReadVector)
			{
				///@TODO: improve this routine with one of polling/select method.
				//Read socket and publish it to the LineGroup 
				if(FD_ISSET(itr->socket().socketfd(), &temprfds))
				{
					Packet *packet = itr->pPacketQueue()->push();
					if(packet)
					{
						packet->setPacketDataLen(itr->socket().readSocket(packet->packetData(), PACKET_DATA_SIZE));
						if(packet->packetDataLen() == -1)
						{
							logMessage(WARN, "Nothing to read on sock : %s. Wrong Alarm", itr->toString().c_str()); 
						}

						packet->_packetAddress = itr;
						packet->_rt = globalClock();

						FeedHandler *fh = packet->_packetAddress->_lineGroup->_feedHandler;
						Recording *recording = fh->_recording;
						if(recording)
						{
							std::shared_ptr<Packet> rpacket= _recordQueue->pushPtr();
							if(rpacket)
							{
								rpacket->_packetAddress = packet->_packetAddress;
								rpacket->_rt = packet->_rt;
								memcpy(rpacket->packetData(), packet->packetData(), packet->packetDataLen());
								rpacket->setPacketDataLen(packet->packetDataLen());
							}
							else
							{
								logMessage(EXCEPTION, "For FeedHandler [ %s : %p ], NetworkReader [ %s : %p ]  recordQueue is full", fh->_name, fh, this->_name, this); 
							}
						}
						packet->pushDeleter();
					}
					else
					{
						logMessage(WARN, "Network write queue for sock : %s is full", itr->toString().c_str()); 
					}
					//Packet is completely writen in the queue 
					//with the deleter specefied in return of pushPtr
					//Packet will be read by lineGroup via queue
				}
			}
		}
		else if(retval < 0)	
		{
			logMessage(TRACE, "select Error in reading, ERROR ID : %d!!!!!", retval);
		}
	}
	logMessage(DEBUG, "NetworkReader [ %s : %p ] thread is stopping now", _name, this);
	_thread->stop();
}

void NetworkReader::networkReaderLoopFile()
{
	//Read ech of the packet from all the registered feedHandlers recording files
	logMessage(INFO, "Starting a Network Loop for NetworkReader [ %s : %p ] in FILE_PLAYBACK mode", _name, this);

	fd_set rfds;

	while(_thread->isActive())
	{
		if(_updateIPVector)
		{
			//Add and remove the sockets that are modified
			updateIPVector();
		}

		int32_t result = 0;
		auto itr = _feedStreams.begin();
		for( ; itr != _feedStreams.end(); itr++)
		{
			result = readPacket(itr->second.front());

			// some error has occured, Please stop
			if(result)
			{
				break;
			}
		}

		if(result)
		{
			//Erase first recording stream and try to get Try to read the next recording.
			std::vector<Recording *>::iterator streamItr = itr->second.begin();
			Recording *rec = *streamItr;
			logMessage(INFO, "NetworkReader [ %s : %p ] will stop reading from recording %s for FeedHandler[%s : %p], Reason code: %d", _name, this, rec->_filename.c_str(), rec->_feedHandler->_name, rec->_feedHandler, result);
			itr->second.erase(streamItr);

			// If no more recording availabale for feed 
			// delete the feed from the map and signal feed to stop now 
			if(itr->second.empty())
			{
				_feedStreams.erase(itr);
				if(_feedStreams.empty())
				{
					// No recording to read signal to stop app
					_stopApp = true;
					_appInstance->stopApp();
				}
			}
			// no more operations on itr as it _feedStreams modified now
		}
	}
	logMessage(DEBUG, "NetworkReader [ %s : %p ] thread is stopping now", _name, this);
	_thread->stop();
}

/**
 * readPacket: Reads the next packet from the recording stream
 * return: if recording ends returns -1
 *			if packet is allowed to publish according to playback mode, then returns
 *			packet with the length
 */
int32_t NetworkReader::readPacket(Recording *recording)
{
	std::string line; 
	streampos currentPos = recording->_in->tellg();

	if(getline(*recording->_in, line))
	{
		if(line.compare(0, 24, CLARICE_HEADER))
		{
			logMessage(EXCEPTION, "FeedHandler [ %s : %p ] Corrupt Recording %s. Will move to next recording", recording->_feedHandler->_name, recording->_feedHandler, recording->_filename.c_str());
			return RECORDING_CORRUPT;
		}
		else
		{
			char data[PACKET_DATA_SIZE];

			vector<string> fields = tokenize(line, ",");
			if(fields.size() < 4)
			{
				fprintf(stderr, "FeedHandler [ %s : %p ] Recording %s : Few fields on the line : %s", recording->_feedHandler->_name, recording->_feedHandler, recording->_filename.c_str(), line.c_str());
				return RECORDING_CORRUPT;
			}

			int length = atoi(fields[LENGTH].c_str());
			int headerLength = fields[HEADER].size() + fields[TIME].size() + fields[IP_PORT].size() + fields[LENGTH].size() + 4;

			uint32_t dataLength = line.size() - headerLength;
			memcpy(data, line.c_str() + headerLength, dataLength);

			while(length > dataLength)
			{
				memcpy(data + dataLength, "\n", 1);
				dataLength++;

				// Next line does not start with CLARICE_HEADER, just write whole line
				std::string lineWithoutHeader;
				if(getline(*recording->_in, lineWithoutHeader))
				{
					memcpy(data + dataLength, lineWithoutHeader.c_str(), lineWithoutHeader.size());
					dataLength += lineWithoutHeader.size();
				}
				else
				{
					fprintf(stderr, "FeedHandler [ %s : %p ] Recording %s : Could not get line without header", recording->_feedHandler->_name, recording->_feedHandler, recording->_filename.c_str());
					return RECORDING_FINISHED_ABNORMALLY;
				}
			}

			if(length != dataLength || length < 0)
			{
				return RECORDING_CORRUPT;
			}

			std:string header = fields[HEADER] + ","+ fields[TIME] + "," + fields[IP_PORT] + "," + fields[LENGTH] + ",";
			if(recording->_reRecord)
			{
				recording->_reRecord->write(header.c_str(), header.size());
				recording->_reRecord->write(data, dataLength);
				recording->_reRecord->write("\n", 1);
			}

			auto packetAddrItr = _ipReadVector.begin();
			for( ; packetAddrItr != _ipReadVector.end(); packetAddrItr++)
			{
				if(static_cast<const IP *>(*packetAddrItr)->toString() == fields[IP_PORT])
				{
					break;
				}
			}

			if(packetAddrItr != _ipReadVector.end())
			{
				const PacketAddress *packetAddr = *packetAddrItr;
				
				// Always spin below for now, as we can not seek back in the recording currently
				Packet *packet = packetAddr->pPacketQueue()->pushSpin();

				if(packet)
				{
					memcpy(packet->packetData(), data, dataLength);
					packet->setPacketDataLen(dataLength);
					if(packet->packetDataLen() == -1)
					{
						logMessage(INFO, "FeedHandler [ %s : %p ] Nothing to read in the packet in recording %s. Empty packet recorded", recording->_feedHandler->_name, recording->_feedHandler, recording->_filename.c_str()); 
					}

					packet->_packetAddress = packetAddr;
					packet->_recordTime.tv_sec = atoll(fields[TIME].c_str());
					packet->_recordTime.tv_nsec = 0;
					uint32_t index = fields[TIME].find_first_of('.');
					if ( index < fields[TIME].size())
					{
						packet->_recordTime.tv_nsec = atoll(fields[TIME].c_str() + index +1 ) * 1000;
					}
					packet->_rt = globalClock();
				
					FeedHandler *fh = packetAddr->_lineGroup->_feedHandler;
					Recording *recording = fh->_recording;
					if(recording)
					{
						Packet *rpacket = _recordQueue->pushSpin();
						if(rpacket)
						{
								rpacket->_packetAddress = packet->_packetAddress;
								//rpacket->_rt = packet->_rt;
								rpacket->_rt.tv_sec = atoll(fields[TIME].c_str());
								rpacket->_rt.tv_nsec = 0;
								uint32_t index = fields[TIME].find_first_of('.');
								if ( index < fields[TIME].size())
								{
									rpacket->_rt.tv_nsec = atoll(fields[TIME].c_str() + index +1 );
								}
								memcpy(rpacket->packetData(), packet->packetData(), packet->packetDataLen());
								rpacket->setPacketDataLen(packet->packetDataLen());
						}
						else
						{
							uint64_t count = recording->_in->gcount();
							// seeking back in the files somehow is not working. anyways not needed as its a file mode
							ASSERT(false, "seeking back in the files somehow is not working.");
							// Going back into the recording file stream so that next time same packet is read
							recording->_in->seekg(currentPos);
							logMessage(EXCEPTION, "For FeedHandler [ %s : %p ], NetworkReader [ %s : %p ]  recordQueue is full. Prev gcount : %llu, Curr gcount %llu, Recording stream is good : %d", fh->_name, fh, this->_name, this, count, recording->_in->gcount(), recording->_in->good()); 
						}
					}
					packet->pushDeleter();
				}
				else
				{
					uint64_t count = recording->_in->gcount();
					// seeking back in the files somehow is not working. anyways not needed as its a file mode
					ASSERT(false, "seeking back in the files somehow is not working.");
					// Going back into the recording file stream so that next time same packet is read
					recording->_in->seekg(currentPos);
					logMessage(EXCEPTION, "For FeedHandler [ %s : %p ], NetworkReader [ %s : %p ] PacketQueue is full for LineGroup [ %s : %p ] in FILE_PLAYBACK mode. Prev gcount : %llu, Curr gcount %llu, Recording stream is good : %d", recording->_feedHandler->_name, recording->_feedHandler, this->_name, this, packetAddr->_lineGroup->_name, packetAddr->_lineGroup, count, recording->_in->gcount(), recording->_in->good()); 
				}
			}
			else
			{
				logMessage(DEBUG, "For FeedHandler [ %s : %p ], No such IP address %s present, Recording: %s", recording->_feedHandler->_name, recording->_feedHandler, fields[IP_PORT].c_str(), recording->_filename.c_str());
			}
		}
		return SUCCESS;
	}
	else
	{
		return RECORDING_FINISHED;
	}
}

/**
 * \brief: updateIPVector
 *			
 * @algo: 
 *		1> Add IPs from _addIPs into _ipReadVector
 *		2> clear _addIPs
 *		3> Remove IPs those IPs from _ipReadVector which are also present in _removeIPs 
 *		4> clear _removeIPs
 *		5> clear _updateIPVector as _ipReadVector is updated now
 */
void NetworkReader::updateIPVector()
{
	WLock lock(_mutexToAddRemove);

	for(auto itr: _addIPs)
	{
		//Check if ip is already present in the vector. If yes, Please raise an exception	
		bool found = false;
			
		for(auto readVectorElement: _ipReadVector)
		{
			if(readVectorElement == itr)
			{
				found = true;
				break;
			}
		}
		
		if(found)
		{
			logMessage(EXCEPTION, "IP [ %s ] is already present", itr->toString().c_str());
		}
		else
		{
			logConsole(DEBUG, "Added IP [ %s ] to the network reader [ %s : %p ]", itr->toString().c_str(), _name, this);
			new ((void *)&itr->socket()) Socket(itr, itr->type());
			_ipReadVector.push_back(itr);
		}
	}
	_addIPs.clear();

	for(auto itr: _removeIPs)
	{
		//Check if ip is already present in the vector. If yes, Please raise an exception	
		bool erased = false;
		auto readVectorItr = _ipReadVector.begin();
		auto readVectorEnd = _ipReadVector.end();

		for(; readVectorItr != readVectorEnd; ++readVectorItr)
		{
			if(*readVectorItr == itr)
			{
				_ipReadVector.erase(readVectorItr);
				logMessage(DEBUG, "Removed IP [ %s ] from NetworkReader [ %s : %p ]", itr->toString().c_str(), _name, this);
				erased = true;
				break;
			}
		}

		if(!erased)
		{
			logMessage(EXCEPTION, "Could not remove IP [ %s ]. NetworkReader [ %s : %p ] was not listening to this IP", itr->toString().c_str(), _name, this);
		}
	}
	_removeIPs.clear();
	_updateIPVector = false;
}

Recording::Recording(const FeedHandler *feed, const std::string &filename, bool write)
	: _in(write? nullptr: new igzstream())
	, _out(write? new ogzstream() : nullptr)
	, _filename(filename)
	, _feedHandler(feed)
	, _reRecord((feed->_reRecord && !write)? new ogzstream() : nullptr)
{
	if(write)
	{
		_out->open(filename.c_str(), ios::out);

		if(!_out->good() )
		{
			///< Throw an error exception duplicate reader name 
			char errorBuff[512];
			snprintf(errorBuff, sizeof(errorBuff), "For FeedHandler [ %s : %p ], could not open playback file %s in write mode, Please check if path exists and have permission to write", _feedHandler->_name, _feedHandler, _filename.c_str());

			std::string error(errorBuff);
			throw std::invalid_argument(error);
		}
	}
	else
	{
		_in->open(filename.c_str(), ios::in);

		if(!_in->good() )
		{
			///< Throw an error exception duplicate reader name 
			char errorBuff[512];
			snprintf(errorBuff, sizeof(errorBuff), "For FeedHandler [ %s : %p ], could not open playback file %s in read mode, Please check if file exists and have permission to read", _feedHandler->_name, _feedHandler, _filename.c_str());

			std::string error(errorBuff);
			throw std::invalid_argument(error);
		}

		if(_reRecord)
		{
			_reRecord->open((filename+".re-record").c_str(), ios::out);
			if(!_reRecord->good() )
			{
				///< Throw an error exception duplicate reader name 
				char errorBuff[512];
				snprintf(errorBuff, sizeof(errorBuff), "For FeedHandler [ %s : %p ], could not open playback file %s in write mode, Please check if file exists and have permission to write", _feedHandler->_name, _feedHandler, (_filename + "re-record").c_str());
				std::string error(errorBuff);
				throw std::invalid_argument(error);
			}

		}
		logConsole(INFO, "For FeedHandler [ %s : %p ] Reading packets from playback file %s in read mode", _feedHandler->_name, _feedHandler, _filename.c_str());
	}
}

Recording::~Recording()
{
// Do not implement the function. It closes the file handles
//	_in->close();
//	delete _in;
}


