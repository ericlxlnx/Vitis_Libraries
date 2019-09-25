/***************************************************************************
Copyright (c) 2016, Xilinx, Inc.
All rights reserved.

Redistribution and use in source and binary forms, with or without modification,
are permitted provided that the following conditions are met:

1. Redistributions of source code must retain the above copyright notice,
this list of conditions and the following disclaimer.

2. Redistributions in binary form must reproduce the above copyright notice,
this list of conditions and the following disclaimer in the documentation
and/or other materials provided with the distribution.

3. Neither the name of the copyright holder nor the names of its contributors
may be used to endorse or promote products derived from this software
without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

 ***************************************************************************/

#ifndef _XF_HOG_DESCRIPTOR_WRAPPER_HPP_
#define _XF_HOG_DESCRIPTOR_WRAPPER_HPP_

#ifndef __cplusplus
#error C++ is needed to include this header
#endif

#include "hls_stream.h"
#include "common/xf_common.h"
#include "common/xf_utility.h"
#include "xf_hog_descriptor_kernel.hpp"

namespace xf {
namespace cv {
template <int WIN_HEIGHT,
          int WIN_WIDTH,
          int WIN_STRIDE,
          int BLOCK_HEIGHT,
          int BLOCK_WIDTH,
          int CELL_HEIGHT,
          int CELL_WIDTH,
          int NOB,
          int DESC_SIZE,
          int IMG_COLOR,
          int OUTPUT_VARIANT,
          int SRC_T,
          int DST_T,
          int ROWS,
          int COLS,
          int NPC = XF_NPPC1,
          bool USE_URAM = false>
void HOGDescriptor(xf::cv::Mat<SRC_T, ROWS, COLS, NPC>& _in_mat, xf::cv::Mat<DST_T, 1, DESC_SIZE, NPC>& _desc_mat) {
    hls::stream<XF_TNAME(SRC_T, NPC)> in_strm;
    hls::stream<XF_CTUNAME(SRC_T, NPC)> in[IMG_COLOR];
    hls::stream<XF_SNAME(XF_576UW)> _block_strm;
    hls::stream<XF_TNAME(DST_T, NPC)> desc_strm;

    //clang-format off
#pragma HLS DATAFLOW
    //clang-format on

    int IN_TC = (ROWS * COLS >> XF_BITSHIFT(NPC));
    for (int i = 0; i < _in_mat.size; i++) {
        //clang-format off
#pragma HLS pipeline ii = 1
#pragma HLS LOOP_TRIPCOUNT min = 1 max = IN_TC
        //clang-format on
        in_strm.write(_in_mat.read(i));
    }

    // Reads the input data from Input stream and writes the data to the output stream
    xFHOGReadFromStream<ROWS, COLS, IMG_COLOR>(in_strm, in, _in_mat.rows, _in_mat.cols);

    // Process function: performs HoG over the input stream and writes the descriptor data to the output stream
    xFDHOG<WIN_HEIGHT, WIN_WIDTH, WIN_STRIDE, BLOCK_HEIGHT, BLOCK_WIDTH, CELL_HEIGHT, CELL_WIDTH, NOB, ROWS, COLS,
           XF_8UP, XF_16UP, XF_NPPC1, XF_8UW, XF_576UW, IMG_COLOR, USE_URAM>(in, _block_strm, _in_mat.rows,
                                                                             _in_mat.cols);

    if (OUTPUT_VARIANT == XF_HOG_RB) {
        const int _desc_size = ((((ROWS - WIN_HEIGHT) / WIN_STRIDE) + 1) * (((COLS - WIN_WIDTH) / WIN_STRIDE) + 1) *
                                ((NOB * (BLOCK_HEIGHT / CELL_HEIGHT) * (BLOCK_WIDTH / CELL_WIDTH)) *
                                 ((WIN_HEIGHT / CELL_HEIGHT) - 1) * ((WIN_WIDTH / CELL_WIDTH) - 1))) >>
                               1;
        assert((DESC_SIZE == _desc_size) &&
               "The input template argument DESC_SIZE must match the '_desc_size' locally computed through the other "
               "input template arguments!");

        // writes the Descriptor data Window wise
        xFWriteHOGDescRB<WIN_HEIGHT, WIN_WIDTH, WIN_STRIDE, CELL_HEIGHT, CELL_WIDTH, NOB, ROWS, COLS, XF_16UP, XF_16UP,
                         XF_NPPC1, XF_576UW, XF_32UW, USE_URAM>(_block_strm, desc_strm, _in_mat.rows, _in_mat.cols);
    } else if (OUTPUT_VARIANT == XF_HOG_NRB) {
        const int _desc_size = ((((ROWS / CELL_HEIGHT) - 1) * ((COLS / CELL_WIDTH) - 1) *
                                 (NOB * (BLOCK_HEIGHT / CELL_HEIGHT) * (BLOCK_WIDTH / CELL_WIDTH))) >>
                                1);
        assert((DESC_SIZE == _desc_size) &&
               "The intput template argument DESC_SIZE must match the '_desc_size' locally computed through the other "
               "input template arguments!");

        // writes the block data and the descriptors are formed on the host
        xFWriteHOGDescNRB<BLOCK_HEIGHT, BLOCK_WIDTH, CELL_HEIGHT, CELL_WIDTH, NOB, XF_DHOG, ROWS, COLS, XF_16UP,
                          XF_NPPC1, XF_576UW, XF_TNAME(DST_T, NPC)>(_block_strm, desc_strm, _in_mat.rows, _in_mat.cols);
    }

    int OUT_TC = (ROWS * COLS >> XF_BITSHIFT(NPC));
    for (int i = 0; i < _desc_mat.size; i++) {
        //clang-format off
#pragma HLS pipeline ii = 1
#pragma HLS LOOP_TRIPCOUNT min = 1 max = IN_TC
        //clang-format on
        _desc_mat.write(i, desc_strm.read());
    }
}
} // namespace cv
} // namespace xf

#endif // _XF_HOG_DESCRIPTOR_WRAPPER_HPP_