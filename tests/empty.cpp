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
#include <gtest/gtest.h>

#include <vcl/graphics/recorder/recorder.h>

using namespace Vcl::Graphics::Recorder;

TEST(RecorderTest, EmptyOutputAviH264)
{
	Recorder rec{ OutputFormat::Avi, CodecType::H264 };
	rec.open("test.avi", 256, 256, 25);
}
TEST(RecorderTest, EmptyOutputMkvH264)
{
	Recorder rec{ OutputFormat::Mkv, CodecType::H264 };
	rec.open("test.mkv", 256, 256, 25);
}
TEST(RecorderTest, EmptyOutputMp4H264)
{
	Recorder rec{ OutputFormat::Mp4, CodecType::H264 };
	rec.open("test.mp4", 256, 256, 25);
}
