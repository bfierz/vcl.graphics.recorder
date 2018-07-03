/* The VCL screen capture library is released under the MIT license.
 * 
 * Copyright(c) 2018 Basil Fierz
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files(the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions :
 * 
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */
#pragma once

// Abseil
#include <absl/strings/string_view.h>

// GSL
#include <gsl/gsl>

#ifdef VCL_GRAPHICS_RECORDER_EXPORTS
#	define VCL_GRAPHICS_RECORDER_API __declspec(dllexport)   
#else  
#	define VCL_GRAPHICS_RECORDER_API __declspec(dllimport)   
#endif 

extern "C"
{
	struct AVCodec;
	struct AVCodecContext;
	struct AVCodecParameters;
	struct AVFormatContext;
	struct AVFrame;
	struct AVStream;
}

namespace Vcl { namespace Graphics { namespace Recorder
{
	enum class OutputFormat
	{
		Avi,
		Mkv,
		Mp4
	};

	enum class Codec
	{
		H264
	};

	class VCL_GRAPHICS_RECORDER_API Recorder
	{
	public:
		Recorder(OutputFormat out_fmt, Codec codec);
		~Recorder();

	public:
		void open(absl::string_view sink_name, unsigned int width, unsigned int height, unsigned int frame_rate);
		void close();

		bool write(gsl::span<const uint8_t> Y, gsl::span<const uint8_t> U, gsl::span<const uint8_t> V);

	private:
		//! Configure specific H264 parameters
		void configureH264();

		//! Hold the formating of the IO container
		AVFormatContext* _fmtCtx{nullptr};

		//! Recording stream
		AVStream* _recStream{nullptr};

		//! Actual codec
		AVCodec* _codec{nullptr};

		//! Codec context
		AVCodecContext* _codecCtx{nullptr};

		//! Is the output open
		bool _isOpen{false};

		//! Temporary frames for data processing
		AVFrame* _processing_frame{nullptr};

		//! Current frame count
		int64_t _frames{0};
	};
}}}
