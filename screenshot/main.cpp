#ifdef _MSC_VER
#pragma comment(linker, "/subsystem:windows")
#endif

#define _ENABLE_ATOMIC_ALIGNMENT_FIX

#include <string>
#include <thread>
#include <atomic>
#include <algorithm>
#include <ctime>

#define NOMINMAX

#include <Windows.h>
#include <Shlobj.h>
#include <jpeglib.h>
#include <png.h>

HGLOBAL to_clipboard(const std::wstring& str)
{
	HGLOBAL handle = GlobalAlloc(GMEM_MOVEABLE, str.size() * 2 + 2);
	std::memcpy(GlobalLock(handle), str.c_str(), str.size() * 2 + 2);
	GlobalUnlock(handle);
	return handle;
}

void save_jpeg(FILE* file, HDC ctx, HBITMAP src, UINT w, UINT h)
{
	// Row buffer
	std::basic_string<unsigned char> row(w * 3, '\0');
	JSAMPROW row_pointer[1] = {&row.front()};

	// Row format
	BITMAPINFO bmp_info = {0};
	bmp_info.bmiHeader.biSize = sizeof(bmp_info.bmiHeader);

	// Verify image format
	GetDIBits(ctx, src, 0, 0, NULL, &bmp_info, DIB_RGB_COLORS);
	bmp_info.bmiHeader.biSize = sizeof(bmp_info.bmiHeader);
	bmp_info.bmiHeader.biBitCount = 24;
	bmp_info.bmiHeader.biCompression = BI_RGB;
	bmp_info.bmiHeader.biWidth = w;
	bmp_info.bmiHeader.biHeight = h;

	// JPEG compression context
	struct jpeg_compress_struct cinfo;
	struct jpeg_error_mgr jerr;
	cinfo.err = jpeg_std_error(&jerr);
	jpeg_create_compress(&cinfo);
	jpeg_stdio_dest(&cinfo, file);

	cinfo.image_width = w;
	cinfo.image_height = h;
	cinfo.input_components = 3;
	cinfo.in_color_space = JCS_RGB;
	jpeg_set_defaults(&cinfo);
	cinfo.num_components = 3;
	cinfo.dct_method = JDCT_FLOAT;
	jpeg_simple_progression(&cinfo);
	jpeg_set_quality(&cinfo, 95, TRUE);
	jpeg_start_compress(&cinfo, TRUE);

	while (cinfo.next_scanline < cinfo.image_height)
	{
		// Get scanline
		GetDIBits(ctx, src, h - 1 - cinfo.next_scanline, 1, &row.front(), &bmp_info, DIB_RGB_COLORS);

		// Remap color components
		for (size_t i = 0; i < w; i++) std::swap(row[i * 3], row[i * 3 + 2]);

		jpeg_write_scanlines(&cinfo, row_pointer, 1);
	}

	jpeg_finish_compress(&cinfo);
	jpeg_destroy_compress(&cinfo);
}

void save_png(FILE* file, HDC ctx, HBITMAP src, UINT w, UINT h)
{
	png_structp png_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);

	png_infop info_ptr = png_create_info_struct(png_ptr);

	png_set_IHDR(png_ptr, info_ptr, w, h, 8,
		PNG_COLOR_TYPE_RGB,
		PNG_INTERLACE_NONE,
		PNG_COMPRESSION_TYPE_DEFAULT,
		PNG_FILTER_TYPE_DEFAULT);

	// Row format
	BITMAPINFO bmp_info = {0};
	bmp_info.bmiHeader.biSize = sizeof(bmp_info.bmiHeader);

	// Verify image format
	GetDIBits(ctx, src, 0, 0, NULL, &bmp_info, DIB_RGB_COLORS);
	bmp_info.bmiHeader.biSize = sizeof(bmp_info.bmiHeader);
	bmp_info.bmiHeader.biBitCount = 24;
	bmp_info.bmiHeader.biCompression = BI_RGB;
	bmp_info.bmiHeader.biWidth = w;
	bmp_info.bmiHeader.biHeight = h;

	// Allocate rows
	png_byte** row_pointers = static_cast<png_byte**>(png_malloc(png_ptr, h * sizeof(png_byte*)));

	for (UINT y = 0; y < h; y++)
	{
		// Allocate single row
		auto row = row_pointers[y] = static_cast<png_byte*>(png_malloc(png_ptr, w * 3));
		
		// Get line
		GetDIBits(ctx, src, h - 1 - y, 1, row, &bmp_info, DIB_RGB_COLORS);

		// Remap color components
		for (size_t i = 0; i < w; i++) std::swap(row[i * 3], row[i * 3 + 2]);
	}

	png_init_io(png_ptr, file);
	png_set_rows(png_ptr, info_ptr, row_pointers);
	png_write_png(png_ptr, info_ptr, PNG_TRANSFORM_IDENTITY, NULL);

	// Cleanup
	for (UINT y = 0; y < h; y++)
	{
		png_free(png_ptr, row_pointers[y]);
	}

	png_free(png_ptr, row_pointers);
	png_destroy_write_struct(&png_ptr, &info_ptr);
}

int CALLBACK wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPWSTR lpCmdLine, int nCmdShow)
{
	if (!RegisterHotKey(NULL, 1, MOD_NOREPEAT, VK_SNAPSHOT))
	{
		MessageBoxW(0, L"Failed to register 'Print Screen'", L"Screenshot Utility", MB_ICONERROR);
		return 1;
	}

	if (!RegisterHotKey(NULL, 2, MOD_WIN | MOD_NOREPEAT, VK_SNAPSHOT))
	{
		MessageBoxW(0, L"Failed to register 'Win + Print Screen'", L"Screenshot Utility", MB_ICONERROR);
		return 1;
	}

	if (!RegisterHotKey(NULL, 3, MOD_ALT | MOD_NOREPEAT, VK_SNAPSHOT))
	{
		MessageBoxW(0, L"Failed to register 'Alt + Print Screen'", L"Screenshot Utility", MB_ICONERROR);
		return 1;
	}

	if (!RegisterHotKey(NULL, 4, MOD_CONTROL | MOD_NOREPEAT, VK_SNAPSHOT))
	{
		MessageBoxW(0, L"Failed to register 'Ctrl + Print Screen'", L"Screenshot Utility", MB_ICONERROR);
		return 1;
	}

	if (!RegisterHotKey(NULL, 5, MOD_SHIFT | MOD_NOREPEAT, VK_SNAPSHOT))
	{
		MessageBoxW(0, L"Failed to register 'Shift + Print Screen'", L"Screenshot Utility", MB_ICONERROR);
		return 1;
	}

	if (!RegisterHotKey(NULL, 6, MOD_SHIFT | MOD_WIN | MOD_NOREPEAT, VK_SNAPSHOT))
	{
		MessageBoxW(0, L"Failed to register 'Shift + Win + Print Screen'", L"Screenshot Utility", MB_ICONERROR);
		return 1;
	}

	if (!RegisterHotKey(NULL, 7, MOD_SHIFT | MOD_ALT | MOD_NOREPEAT, VK_SNAPSHOT))
	{
		MessageBoxW(0, L"Failed to register 'Shift + Alt + Print Screen'", L"Screenshot Utility", MB_ICONERROR);
		return 1;
	}

	if (!RegisterHotKey(NULL, 8,  MOD_SHIFT | MOD_CONTROL | MOD_NOREPEAT, VK_SNAPSHOT))
	{
		MessageBoxW(0, L"Failed to register 'Shift + Ctrl + Print Screen'", L"Screenshot Utility", MB_ICONERROR);
		return 1;
	}

	// Determine working directory
	std::wstring root(256, L'\0');
	root.resize(GetCurrentDirectoryW(257, &root.front()));

	const auto tid = GetCurrentThreadId();
	std::atomic<bool> scroll_state{false};
	std::atomic<bool> control_state{false};
	std::atomic<POINT> point_1;

	std::thread monitor_thread([&]()
	{
		while (true)
		{
			Sleep(5);

			// Monitor Scroll Lock state
			if (scroll_state && GetKeyState(VK_SCROLL) >= 0)
			{
				scroll_state = false;
				::keybd_event(VK_SCROLL, 0x46, KEYEVENTF_EXTENDEDKEY, 0);
				::keybd_event(VK_SCROLL, 0x46, KEYEVENTF_EXTENDEDKEY | KEYEVENTF_KEYUP, 0);
			}
			else if (!scroll_state && GetKeyState(VK_SCROLL) & 1)
			{
				// Scroll Lock detected: send message (TODO)
				scroll_state = true;
				PostThreadMessageW(tid, WM_HOTKEY, 0, VK_SNAPSHOT << 16);
			}

			// Monitor Ctrl state
			if (control_state && GetKeyState(VK_CONTROL) >= 0)
			{
				control_state = false;
			}
			else if (!control_state && GetKeyState(VK_CONTROL) < 0)
			{
				control_state = true;
				POINT point;
				GetCursorPos(&point);
				point_1 = point;
			}

			// Monitor the clipboard in order to augment file copying
			if (!control_state && OpenClipboard(0))
			{
				// Get file list object
				auto hdrop = GetClipboardData(CF_HDROP);

				// Append synthesized filename
				if (hdrop && !GetClipboardData(CF_UNICODETEXT))
				{
					std::wstring list;

					// Read file list
					LPVOID ptr = GlobalLock(hdrop);
					LPDROPFILES pdf = static_cast<LPDROPFILES>(ptr);

					// Unicode only
					if (pdf->fWide)
					{
						PCZZWSTR nnts = reinterpret_cast<PCZZWSTR>(static_cast<PBYTE>(ptr) + pdf->pFiles);

						// Parse strings
						while (auto& ch0 = *nnts)
						{
							// Parse chars
							for (; *nnts; nnts++)
							{
							}

							if (nnts - &ch0 + 0ull >= root.size() && std::wcsncmp(&ch0, root.data(), root.size()) == 0)
							{
								if (!list.empty())
								{
									// Append delimiter if necessary
									list.push_back(L'\n');
								}

								// Append prefix
								list.append(lpCmdLine);

								// Append filename
								for (auto pch = &ch0 + root.size(); pch != nnts; pch++)
								{
									switch (auto ch = *pch)
									{
									case L' ':
									{
										list += L"%20";
										break;
									}
									case L'\\':
									{
										list += L'/';
										break;
									}
									default:
									{
										list += ch;
										break;
									}
									}
								}
							}

							nnts++;
						}
					}

					GlobalUnlock(hdrop);

					SetClipboardData(CF_UNICODETEXT, to_clipboard(list));
				}
				
				CloseClipboard();
			}
		}
	});

	MSG msg = {0};

	while (GetMessageW(&msg, NULL, 0, 0) != 0)
	{
		if (msg.message == WM_HOTKEY && HIWORD(msg.lParam) == VK_SNAPSHOT)
		{
			const HWND fw = GetForegroundWindow();

			if (const HDC fdc = (msg.lParam & MOD_ALT ? GetDC(fw) : GetDC(0)))
			{
				if (const HDC hdc = CreateCompatibleDC(fdc))
				{
					RECT rc;

					if (msg.lParam & MOD_ALT)
					{
						GetClientRect(fw, &rc);
					}
					else if (msg.lParam & MOD_WIN)
					{
						GetClientRect(fw, &rc);
						ClientToScreen(fw, reinterpret_cast<POINT*>(&rc.left));
						ClientToScreen(fw, reinterpret_cast<POINT*>(&rc.right));
					}
					else if (msg.lParam & MOD_CONTROL)
					{
						POINT point1;
						point1 = point_1.load();
						POINT point2;
						GetCursorPos(&point2);

						rc.left = std::min(point1.x, point2.x);
						rc.top = std::min(point1.y, point2.y);
						rc.right = std::max(point1.x, point2.x);
						rc.bottom = std::max(point1.y, point2.y);
					}
					else
					{
						GetWindowRect(fw, &rc);
					}

					LONG w = rc.right - rc.left;
					LONG h = rc.bottom - rc.top;

					if (HBITMAP hbmp = CreateCompatibleBitmap(fdc, w, h))
					{
						// OMG copy bits
						SelectObject(hdc, hbmp);
						BitBlt(hdc, 0, 0, w, h, fdc, rc.left, rc.top, SRCCOPY);

						// Get current time
						std::time_t _time = std::time(nullptr);
						struct tm timep;
						::localtime_s(&timep, &_time);

						// Format time
						std::wstring name(256, L'\0');
						name.resize(std::wcsftime(&name.front(), name.size() + 1, L"/capture-%F-%H%M%S", &timep));

						// Encoder function
						decltype(save_png)* save_func = {0};

						// Save the bitmap
						if (msg.lParam & MOD_SHIFT)
						{
							name += L".png";
							save_func = save_png;
						}
						else
						{
							name += L".jpg";
							save_func = save_jpeg;
						}

						FILE* outfile = {0};
						if (errno_t err = _wfopen_s(&outfile, (root + name).c_str(), L"wb"))
						{
							MessageBoxW(0, (L"Failed to create file:\n" + root + name).c_str(), L"Screenshot Utility", MB_ICONERROR);
						}
						else
						{
							save_func(outfile, hdc, hbmp, w, h);
							fclose(outfile);
						}

						// Prepare the filename (append desired root)
						name.insert(0, lpCmdLine);
						HGLOBAL hMem = to_clipboard(name);

						// Put the bitmap and formatted file name into the clipboard
						while (!OpenClipboard(0)) Sleep(1);
						EmptyClipboard();
						SetClipboardData(CF_UNICODETEXT, hMem);
						SetClipboardData(CF_BITMAP, hbmp);
						CloseClipboard();

						DeleteObject(hbmp);
					}

					DeleteObject(hdc);
				}

				ReleaseDC(fw, fdc);
			}
		}
	}

	monitor_thread.join();
	return 0;
}
