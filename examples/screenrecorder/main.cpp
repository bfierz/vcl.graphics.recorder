/*
 * This file is part of the Visual Computing Library (VCL) release under the
 * MIT license.
 *
 * Copyright (c) 2018 Basil Fierz
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

// C++ standard library
#include <atomic>
#include <memory>
#include <thread>
#include <vector>

// WinSDK
#define NOMINMAX
#include <windows.h>

// VCL recorder
#include <vcl/graphics/recorder/recorder.h>

#include "application.h"

class CallBackTimer
{
public:
    CallBackTimer()
    :_execute(false)
    {}

    ~CallBackTimer()
	{
        if (_execute.load(std::memory_order_acquire))
            stop();
    }

    void stop()
    {
        _execute.store(false, std::memory_order_release);
        if (_thd.joinable())
            _thd.join();
    }

    void start(int interval, std::function<void(void)> func)
    {
        if (_execute.load(std::memory_order_acquire))
            stop();

        _execute.store(true, std::memory_order_release);
        _thd = std::thread([this, interval, func]()
        {
            while (_execute.load(std::memory_order_acquire))
			{
				const auto before = std::chrono::steady_clock::now();
                func();
				const auto elapsed = std::chrono::steady_clock::now() - before;
				const auto sleep = std::chrono::milliseconds(interval) - elapsed;
				std::cout << elapsed.count() / 1000000 << std::endl;

                std::this_thread::sleep_for(sleep);
            }
        });
    }

    bool is_running() const noexcept
	{
        return _execute.load(std::memory_order_acquire) && _thd.joinable();
    }

private:
    std::atomic<bool> _execute;
    std::thread _thd;
};

class Screen
{
public:
	Screen(POINT upper_left, POINT lower_right)
		: _bitmapInfo{0}
		, _ul(upper_left)
		, _lr(lower_right)
	{
		LONG w = std::abs(_lr.x-_ul.x);
		LONG h = std::abs(_lr.y-_ul.y);

		_screen  = GetDC(0);
		_dc      = CreateCompatibleDC(_screen);
		_bitmap  = CreateCompatibleBitmap(_screen, w, h);
		
		// Allocate the memory of the portion of the screen to copy
		_bitmapInfo.bmiHeader.biSize = sizeof(_bitmapInfo.bmiHeader);
		if (GetDIBits(_dc, _bitmap, 0, 0, NULL, &_bitmapInfo, DIB_RGB_COLORS) == 0)
			return;

		// Request image data with inverted line-order for easier iteration
		_bitmapInfo.bmiHeader.biBitCount = 24;
		_bitmapInfo.bmiHeader.biCompression = BI_RGB;
		_bitmapInfo.bmiHeader.biSizeImage = 3*w*h;
		_bitmapInfo.bmiHeader.biHeight = -h;

		_screenCopy = std::make_unique<uint8_t[]>(_bitmapInfo.bmiHeader.biSizeImage);
	}
	~Screen()
	{
		DeleteObject(_bitmap);
		DeleteDC(_dc);
		ReleaseDC(0, _screen);
	}

	bool bitBlit()
	{
		LONG w = std::abs(_lr.x-_ul.x);
		LONG h = std::abs(_lr.y-_ul.y);

		HGDIOBJ old_obj = SelectObject(_dc, _bitmap);
		
		BOOL blitSuccess = BitBlt(_dc, 0, 0, w, h, _screen, _ul.x, _ul.y, SRCCOPY);
		BOOL copySuccess = GetDIBits(_dc, _bitmap, 0, h, _screenCopy.get(), &_bitmapInfo, DIB_RGB_COLORS);

		SelectObject(_dc, old_obj);

		return blitSuccess != 0 && copySuccess != 0;
	}

	unsigned int width() const
	{
		return gsl::narrow_cast<unsigned int>(std::abs(_lr.x-_ul.x));
	}
	
	unsigned int height() const
	{
		return gsl::narrow_cast<unsigned int>(std::abs(_lr.y-_ul.y));
	}

	gsl::span<std::array<uint8_t, 3>> screenBuffer() const
	{
		return gsl::make_span(reinterpret_cast<std::array<uint8_t, 3>*>(_screenCopy.get()), _bitmapInfo.bmiHeader.biSizeImage);
	}

private:
	//! Handle to the screen
	HDC _screen;

	//! Device context of the screen
	HDC _dc;

	//! Bitmap header of the screen copy
	BITMAPINFO _bitmapInfo;

	//! Bitmap resouces to store a copy of the screen
	HBITMAP _bitmap;

	//! Upper-left corner
	POINT _ul;

	//! Lower-right corner
	POINT _lr;

	//! Copy of the screen portion
	std::unique_ptr<uint8_t[]> _screenCopy;
};

using namespace Vcl::Graphics::Recorder;

// Application state
std::unique_ptr<Recorder> recorder;

// Object handle screen capture
std::unique_ptr<Screen> screen;

// Trigger recording
CallBackTimer recording_timer;

void record(Recorder* recorder, Screen* screen)
{
	if (screen->bitBlit())
	{
		recorder->write(screen->screenBuffer(), screen->width(), screen->height());
	}
}

void startRecording(int width, int height)
{	
	screen = std::make_unique<Screen>(POINT{0, 0}, POINT{1920, 1080});

	recorder = std::make_unique<Recorder>(OutputFormat::Mp4, CodecType::H264);
	recorder->open("screen_capture.mp4", width, height, 25);
	recording_timer.start(40, []()
	{
		record(recorder.get(), screen.get());
	});
}
void stopRecording()
{
	recording_timer.stop();
	recorder->close();
	recorder.reset();
}

void renderUI(Application& app)
{
	ImGuiWindowFlags corner =
		ImGuiWindowFlags_NoMove |
		ImGuiWindowFlags_NoResize |
		ImGuiWindowFlags_NoCollapse |
		ImGuiWindowFlags_NoSavedSettings |
		ImGuiWindowFlags_AlwaysAutoResize |
		ImGuiWindowFlags_NoTitleBar;

	ImGui::Begin("Commands", nullptr, corner);
	ImGui::SetWindowPos({ 0, 0 });
	
	if (ImGui::Button("Record"))
	{
		if (recorder == nullptr)
			startRecording(1920, 1080);
		else
			stopRecording();
	}

	ImGui::End();
}

int main(int /* argc */, char ** /* argv */)
{
	Application app{ "VCL Screen-recorder", 768, 768 };
	app.setUIDrawCallback(renderUI);

	return app.run();
}
