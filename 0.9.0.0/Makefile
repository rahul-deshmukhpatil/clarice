.SUFFIXES: .o .cpp
include main.mk
include sources

INFRA_OBJECTS = ${INFRA_SOURCES:.cpp=.o}
INFRA_DEBUG_OBJECTS = ${INFRA_OBJECTS:%.o=${DEBUG_OBJ_DIR}/%.o}
INFRA_RELEASE_OBJECTS = ${INFRA_OBJECTS:%.o=${RELEASE_OBJ_DIR}/%.o}

CORE_OBJECTS = ${CORE_SOURCES:.cpp=.o}
CORE_DEBUG_OBJECTS = ${CORE_OBJECTS:%.o=${DEBUG_OBJ_DIR}/%.o}
CORE_RELEASE_OBJECTS = ${CORE_OBJECTS:%.o=${RELEASE_OBJ_DIR}/%.o}

BASE_OBJECTS = ${BASE_SOURCES:.cpp=.o}
BASE_DEBUG_OBJECTS = ${BASE_OBJECTS:%.o=${DEBUG_OBJ_DIR}/%.o}
BASE_RELEASE_OBJECTS = ${BASE_OBJECTS:%.o=${RELEASE_OBJ_DIR}/%.o}

MDAPP_OBJECTS = ${MDAPP_SOURCES:.cpp=.o}
MDAPP_DEBUG_OBJECTS = ${MDAPP_OBJECTS:%.o=${DEBUG_OBJ_DIR}/%.o}
MDAPP_RELEASE_OBJECTS= ${MDAPP_OBJECTS:%.o=${DEBUG_OBJ_DIR}/%.o}


MILLENIUM_OBJECTS = ${MILLENIUM_SOURCES:.cpp=.o}
MILLENIUM_DEBUG_OBJECTS = ${MILLENIUM_OBJECTS:%.o=${DEBUG_OBJ_DIR}/%.o}
MILLENIUM_RELEASE_OBJECTS = ${MILLENIUM_OBJECTS:%.o=${RELEASE_OBJ_DIR}/%.o}

OMXNORDIC_OBJECTS = ${OMXNORDIC_SOURCES:.cpp=.o}
OMXNORDIC_DEBUG_OBJECTS = ${OMXNORDIC_OBJECTS:%.o=${DEBUG_OBJ_DIR}/%.o}
OMXNORDIC_RELEASE_OBJECTS = ${OMXNORDIC_OBJECTS:%.o=${RELEASE_OBJ_DIR}/%.o}

$(DEBUG_OBJ_DIR)/%.o : %.cpp
	test -d $(dir $@) || mkdir -p $(dir $@)
	${CC} ${CFLAGS} ${CORE_USER_FLAGS} ${CORE_C_DEBUG_FLAGS} ${CD_INC_DIR} $< -o $@

$(RELEASE_OBJ_DIR)/%.o : %.cpp
	test -d $(dir $@) || mkdir -p $(dir $@)
	${CC} ${CFLAGS} ${CORE_USER_FLAGS} ${CORE_C_RELEASE_FLAGS} ${CD_INC_DIR} $< -o $@

infra-debug: ${INFRA_DEBUG_OBJECTS}
	${CC} -m${HOSTBITS} ${CORE_C_DEBUG_FLAGS} -shared -o ${DEBUG_OBJ_DIR}/libcd-infra-dbg.so ${INFRA_DEBUG_OBJECTS} ${CD_STD_LIBS}
	mkdir -p ${CD_LIB_DIR}
	mv ${DEBUG_OBJ_DIR}/*.so ${CD_LIB_DIR}
	ln -sf ${CD_LIB_DIR}/libcd-infra-dbg.so ${CD_LIB_DIR}/libcd-infra.so
    
infra-release: ${INFRA_RELEASE_OBJECTS}
	${CC}  -m${HOSTBITS} ${CORE_C_RELEASE_FLAGS} -shared -o ${RELEASE_OBJ_DIR}/libcd-infra-release.so ${INFRA_RELEASE_OBJECTS} ${CD_STD_LIBS} 
	mkdir -p ${CD_LIB_DIR}
	mv ${RELEASE_OBJ_DIR}/libcd-infra-release.so ${CD_LIB_DIR}
	ln -sf ${CD_LIB_DIR}/libcd-infra-release.so ${CD_LIB_DIR}/libcd-infra.so

base-debug: ${BASE_DEBUG_OBJECTS}
	${CC} -m${HOSTBITS} ${CORE_C_DEBUG_FLAGS} -shared -o ${DEBUG_OBJ_DIR}/libcd-base-dbg.so ${BASE_DEBUG_OBJECTS} ${CD_STD_LIBS} ${CD_BOOST_LIBS}
	mkdir -p ${CD_LIB_DIR}
	mv ${DEBUG_OBJ_DIR}/*.so ${CD_LIB_DIR}
	ln -sf ${CD_LIB_DIR}/libcd-base-dbg.so ${CD_LIB_DIR}/libcd-base.so
    
base-release: ${BASE_RELEASE_OBJECTS}
	${CC}  -m${HOSTBITS} ${CORE_C_RELEASE_FLAGS} -shared -o ${RELEASE_OBJ_DIR}/libcd-base-release.so ${BASE_RELEASE_OBJECTS} ${CD_STD_LIBS} ${CD_BOOST_LIBS} 
	mkdir -p ${CD_LIB_DIR}
	mv ${RELEASE_OBJ_DIR}/libcd-base-release.so ${CD_LIB_DIR}
	ln -sf ${CD_LIB_DIR}/libcd-base-release.so ${CD_LIB_DIR}/libcd-base.so

millenium-debug: ${MILLENIUM_DEBUG_OBJECTS}
	${CC} -m${HOSTBITS} ${CORE_C_DEBUG_FLAGS} -shared -o ${DEBUG_OBJ_DIR}/libcd-millenium-dbg.so ${MILLENIUM_DEBUG_OBJECTS} ${CD_STD_LIBS} 
	mkdir -p ${CD_LIB_DIR}
	mv ${DEBUG_OBJ_DIR}/*.so ${CD_LIB_DIR}
	ln -sf ${CD_LIB_DIR}/libcd-millenium-dbg.so ${CD_LIB_DIR}/libcd-millenium.so

millenium-release: ${MILLENIUM_RELEASE_OBJECTS}
	${CC}  -m${HOSTBITS} ${CORE_C_RELEASE_FLAGS} -shared -o ${RELEASE_OBJ_DIR}/libcd-millenium-release.so ${MILLENIUM_RELEASE_OBJECTS} ${CD_STD_LIBS} 
	mkdir -p ${CD_LIB_DIR}
	mv ${RELEASE_OBJ_DIR}/libcd-millenium-release.so ${CD_LIB_DIR}
	ln -sf ${CD_LIB_DIR}/libcd-millenium-release.so ${CD_LIB_DIR}/libcd-millenium.so

omxnordic-debug: ${OMXNORDIC_DEBUG_OBJECTS}
	${CC} -m${HOSTBITS} ${CORE_C_DEBUG_FLAGS} -shared -o ${DEBUG_OBJ_DIR}/libcd-omxnordic-dbg.so ${OMXNORDIC_DEBUG_OBJECTS} ${CD_STD_LIBS} 
	mkdir -p ${CD_LIB_DIR}
	mv ${DEBUG_OBJ_DIR}/*.so ${CD_LIB_DIR}
	ln -sf ${CD_LIB_DIR}/libcd-omxnordic-dbg.so ${CD_LIB_DIR}/libcd-omxnordic.so

omxnordic-release: ${OMXNORDIC_RELEASE_OBJECTS}
	${CC}  -m${HOSTBITS} ${CORE_C_RELEASE_FLAGS} -shared -o ${RELEASE_OBJ_DIR}/libcd-omxnordic-release.so ${OMXNORDIC_RELEASE_OBJECTS} ${CD_STD_LIBS} 
	mkdir -p ${CD_LIB_DIR}
	mv ${RELEASE_OBJ_DIR}/libcd-omxnordic-release.so ${CD_LIB_DIR}
	ln -sf ${CD_LIB_DIR}/libcd-omxnordic-release.so ${CD_LIB_DIR}/libcd-omxnordic.so

mdapp-debug:${MDAPP_DEBUG_OBJECTS}
	${CC} -m${HOSTBITS} -o $@ ${MDAPP_DEBUG_OBJECTS} -L${CD_LIB_DIR} ${CD_CORE_LIBS} ${CD_STD_LIBS} ${CD_BOOST_LIBS}
	mkdir -p ${CD_BIN_DIR}
	mv $@ ${CD_BIN_DIR}/

test:
	mdapp-debug mdapp-release

mdapp-release:${MDAPP_RELEASE_OBJECTS}
	${CC} -m${HOSTBITS} ${CORE_C_RELEASE_FLAGS} -o $@ ${MDAPP_RELEASE_OBJECTS} -L${CD_LIB_DIR} ${CD_CORE_LIBS} ${CD_STD_LIBS} ${CD_BOOST_LIBS} 
	mkdir -p ${CD_BIN_DIR}
	mv $@ ${CD_BIN_DIR}/

infra-clean:
	rm -rf ${INFRA_DEBUG_OBJECTS} ${INFRA_RELEASE_OBJECTS}

base-clean:
	rm -rf ${BASE_DEBUG_OBJECTS} ${BASE_RELEASE_OBJECTS}

millenium-clean:
	rm -rf ${MILLENIUM_DEBUG_OBJECTS} ${MILLENIUM_RELEASE_OBJECTS}

mdapp-clean:
	rm -rf ${CD_BIN_DIR}/mdapp

clean:
	rm -rf ${DEBUG_OBJ_DIR} ${RELEASE_OBJ_DIR} 

all-debug: clean infra-release  base-release millenium-release omxnordic-release mdapp-release 

all-release: clean infra-release  base-release millenium-release omxnordic-release mdapp-release 

