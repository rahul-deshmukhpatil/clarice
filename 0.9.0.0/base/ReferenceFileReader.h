#ifndef __REFERENCEFILEREADER_H__
#define __REFERENCEFILEREADER_H__

struct ReferenceFileRowField
{
    ProductInfoFieldType*	_pInfoFieldType;
    unsigned int            _length;
};

class ReferenceFileReader
{
    public:
        ReferenceFileReader() {}

        /**
         *  parse the REFERENCEFILE file.
         *  @input:
         *  @output:
         *  @return:
         */
        parse(ReferenceFileRowField *rowFields, unsigned int noOfFields, char delimeter, ProductInfos)
        {

            for (unsigned int i = 0; i < noOfFields; i++)
            {
                switch (rowFields[i].pInfoFieldType)
                {
                    case ProductInfoFieldType_Unknown:
                        break;

                    case ProductInfoFieldType_Symbol:
                        break;

                    case ProductInfoFieldType_LotSize:
                        break;

                    case ProductInfoFieldType_TickSize:
                        break;

                    case ProductInfoFieldType_Currency:
                        break;

                    case Default:
                        /**
                         * @TODO: Log an ERROR over here and Stop
                         * Something is wrong, Set the rowFields in array correctly
                         * "Line %d RowField [%d:%s], ProductInfoFieldType and Length [%d,%d]",
                         */
                        break;
                };
            }

            /**
             * @TODO: See if this is necessory, otherwise remove comment.
             * ProductInfo Created. Invoke Feed Specefic function to process product Info.
             * Currently leaving empty.
             */
        }
    private:
};
#endif //__REFERENCEFILEREADER_H__
