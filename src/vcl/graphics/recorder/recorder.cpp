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
#include "recorder.h"

// C++ standard library
#include <exception>

extern "C"
{
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/error.h>
#include <libavutil/imgutils.h>
#include <libavutil/opt.h>
}

namespace Vcl { namespace Graphics { namespace Recorder
{
	Recorder::Recorder(OutputFormat out_fmt, Codec codec)
	{
		_fmtCtx = avformat_alloc_context();
		if (_fmtCtx == nullptr) {
			throw std::domain_error("Cannot allocate AVFormatContext");
		}

		AVOutputFormat* fmt = nullptr;
		if (out_fmt == OutputFormat::Avi)
			fmt = av_guess_format("avi", nullptr, nullptr);
		else if (out_fmt == OutputFormat::Mkv)
			fmt = av_guess_format("matroska", nullptr, nullptr);
		else if (out_fmt == OutputFormat::Mp4)
			fmt = av_guess_format("mp4", nullptr, nullptr);
		else
			throw std::domain_error("Invalid output format definition");

		if (fmt == nullptr)
			throw std::runtime_error("Unable to allocate AVOutputFormat");
		_fmtCtx->oformat = fmt;

		if (codec == Codec::H264)
		{
			_codec = avcodec_find_encoder(AV_CODEC_ID_H264);
			//_codec = avcodec_find_encoder_by_name("libx264");
			//_codec = avcodec_find_encoder_by_name("h264_nvenc");
		}
		else
			throw std::domain_error("Invalid codec definition");

		if (!_codec)
			throw std::runtime_error("Encoder for requested codec not found");
		_codecCtx = avcodec_alloc_context3(_codec);
		if (_fmtCtx->oformat->flags & AVFMT_GLOBALHEADER)
			_codecCtx->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;

		_codecCtx->sample_fmt = _codec->sample_fmts ? _codec->sample_fmts[0] : AV_SAMPLE_FMT_S16;
		configureH264();

		if (!(_recStream = avformat_new_stream(_fmtCtx, nullptr)))
			throw std::runtime_error("Failed creating recording stream");

		_fmtCtx->pb = nullptr;
	}
	Recorder::~Recorder()
	{
		close();

		avcodec_free_context(&_codecCtx);
		avformat_free_context(_fmtCtx);
	}

	void Recorder::open(absl::string_view sink_name, unsigned int width, unsigned int height, unsigned int frame_rate)
	{
		int av_err = -1;

		if (_isOpen)
			throw std::runtime_error("Video is already open");

		if (_fmtCtx->pb != nullptr)
		{
			avio_close(_fmtCtx->pb);
			_fmtCtx->pb = nullptr;
		}

		// Set the output name
		const auto sink_name_len = sink_name.size() + 1;
		_fmtCtx->url = static_cast<char*>(av_malloc(sink_name_len));
		memset(_fmtCtx->url, 0, sink_name_len);
		sink_name.copy(_fmtCtx->url, sink_name.size());

		_recStream->time_base = { 1, static_cast<int>(frame_rate) };

		_codecCtx->width = width;
		_codecCtx->height = height;
		_codecCtx->time_base = _recStream->time_base;
		_codecCtx->pix_fmt = AV_PIX_FMT_YUV420P;

		// Open the codec and prepare for using it
		av_err = avcodec_open2(_codecCtx, _codec, nullptr);
		if (av_err < 0)
			throw std::runtime_error("Opening codec failed");

		// Transfer context to stream
		av_err = avcodec_parameters_from_context(_recStream->codecpar, _codecCtx);
		if (av_err < 0)
			throw std::runtime_error("Extracting codec parameters failed");

		// Debug output
		av_dump_format(_fmtCtx, 0, _fmtCtx->url, 1);

		av_err = avio_open(&_fmtCtx->pb, _fmtCtx->url, AVIO_FLAG_WRITE);
		if (av_err < 0)
			throw std::runtime_error("Opening audio failed");

		AVDictionary* fmt_opts = nullptr;

		av_err = avformat_write_header(_fmtCtx, &fmt_opts);
		if (av_err < 0) {
			if (av_err == AVERROR_INVALIDDATA)
				throw std::runtime_error("Writing AV header failed: Invalid data");
			else
				throw std::runtime_error("Writing AV header failed");
		}

		_isOpen = true;
		_frames = 0;

		// Prepare a frame to be used to compress frames
		_processing_frame = av_frame_alloc();
		if (!_processing_frame)
			throw std::runtime_error("Allocating processing frame failed");

		// Make sure the encoder doesn't keep ref to this frame as we'll modify it.
		av_frame_make_writable(_processing_frame);
		_processing_frame->format = _codecCtx->pix_fmt;
		_processing_frame->width = _codecCtx->width;
		_processing_frame->height = _codecCtx->height;
		av_frame_get_buffer(_processing_frame, 32);

		//av_err = av_image_alloc(_processing_frame->data, _processing_frame->linesize, _codecCtx->width, _codecCtx->height, _codecCtx->pix_fmt, 32);
		if (av_err < 0)
			throw std::runtime_error("Allocating memory for processing frame failed");
	}

	void Recorder::close()
	{
		if (_isOpen)
		{
			av_write_trailer(_fmtCtx);
			avio_close(_fmtCtx->pb);
			_fmtCtx->pb = nullptr;
		}

		if (_processing_frame)
		{
			//av_freep(&_processing_frame->data[0]);
			av_frame_free(&_processing_frame);
		}

		_isOpen = false;
	}

	void Recorder::configureH264()
	{
		int av_err = -1;

		_codecCtx->bit_rate = 400000;
		_codecCtx->gop_size = 12;
		_codecCtx->level = 31;
		_codecCtx->max_b_frames = 1;

		// libx264 specific setting
		av_err = av_opt_set(_codecCtx->priv_data, "crf", "12", 0);
		if (av_err < 0)
			throw std::runtime_error("AV set option crf");

		av_err = av_opt_set(_codecCtx->priv_data, "profile", "main", 0);
		if (av_err < 0)
			throw std::runtime_error("AV set option profile");

		av_err = av_opt_set(_codecCtx->priv_data, "preset", "slow", 0);
		if (av_err < 0)
			throw std::runtime_error("AV set option preset");

		// Disable b-pyramid. CLI options for this is "-b-pyramid 0"
		// Quicktime (ie. iOS) doesn't support this option
		//av_err = av_opt_set(_codecCtx->priv_data, "b-pyramid", "0", 0);
		//if (av_err < 0)
		//	throw std::runtime_error("AV set option b-pyramid");

		const uint8_t spspps[] =
		{
			0x00, 0x00, 0x00, 0x01, 0x67, 0x42, 0x00, 0x0a, 0xf8, 0x41, 0xa2,
			0x00, 0x00, 0x00, 0x01, 0x68, 0xce, 0x38, 0x80
		};

		_codecCtx->extradata = (uint8_t *)av_malloc(sizeof(uint8_t) * sizeof(spspps));
		for (unsigned int index = 0; index < sizeof(spspps); index++)
		{
			_codecCtx->extradata[index] = spspps[index];
		}
		_codecCtx->extradata_size = (int)sizeof(spspps);
	}

	bool Recorder::write(gsl::span<const uint8_t> Y, gsl::span<const uint8_t> U, gsl::span<const uint8_t> V)
	{
		// Fill the processing frame
		av_image_fill_arrays(_processing_frame->data, _processing_frame->linesize, nullptr, (AVPixelFormat)_processing_frame->format, _processing_frame->width, _processing_frame->height, 1);
		_processing_frame->data[0] = const_cast<uint8_t*>(Y.data());
		_processing_frame->data[1] = const_cast<uint8_t*>(U.data());
		_processing_frame->data[2] = const_cast<uint8_t*>(V.data());
		_processing_frame->pts = _frames++;

		// Create a packet for the codec
		AVPacket pkt = { 0 };
		av_init_packet(&pkt);
		av_packet_rescale_ts(&pkt, _codecCtx->time_base, _recStream->time_base);

		int av_err = avcodec_send_frame(_codecCtx, _processing_frame);
		if (av_err < 0)
			return false;
		av_err = avcodec_send_frame(_codecCtx, nullptr);
		if (av_err < 0)
			return false;

		int got_output = 0;
		av_err = avcodec_receive_packet(_codecCtx, &pkt);
		if (av_err == AVERROR(EAGAIN) || av_err == AVERROR_EOF)
			return false;
		else if (!av_err)
			got_output = 1;
		
		if (got_output != 0)
		{
			av_packet_rescale_ts(&pkt, _codecCtx->time_base, _recStream->time_base);
			pkt.stream_index = _recStream->index;

			av_err = av_interleaved_write_frame(_fmtCtx, &pkt);
			if (av_err < 0)
				return false;

			av_packet_unref(&pkt);
			return true;
		}
		
		return false;
	}
}}}
