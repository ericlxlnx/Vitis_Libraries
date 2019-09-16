unset SDSOC_SDK
unset SDSOC_VIVADO
unset SDSOC_VIVADO_HLS
unset SDX_VIVADO
unset SDX_VIVADO_HLS
unset XILINX_VIVADO_HLS
unset XILINX_SDK
export VIVADO_TEST_BUILD="2019.2"
export VIVADO_TEST_BUILD_DATE="_daily_latest"
export VIVADO_TEST_VERSION="$VIVADO_TEST_BUILD$VIVADO_TEST_BUILD_DATE"

export TA_PATH=/proj/xbuilds/$VIVADO_TEST_VERSION/installs/lin64

export XILINX_VIVADO=${TA_PATH}/Vivado/$VIVADO_TEST_BUILD

source ${TA_PATH}/Vivado/$VIVADO_TEST_BUILD/settings64.sh