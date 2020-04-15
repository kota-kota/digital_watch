#include "DWType.hpp"
#include "DWMain.hpp"
#include "DWUtility.hpp"

#include <Windows.h>
#include <tchar.h>
#include <cstdio>


//定数定義
namespace {

	//ウィンドウ名
	static const TCHAR* WIN_NAME = TEXT("ExDigitalWatch");

	//ウィンドウ位置
	static const std::int32_t WIN_POSX = 0;
	static const std::int32_t WIN_POSY = 0;

	//ウィンドウ幅高さ
	static const std::int32_t WIN_WIDTH = 500;
	static const std::int32_t WIN_HEIGHT = 200;
}

//内部関数
namespace {

	//WM_CREATEイベント処理
	void WndProc_WMCreate(HWND hWnd)
	{
		//DWWindow作成
		dw::DWWindow::create(hWnd);

		//DWMain開始
		dw::DWMain::start();
	}

	//WM_DESTROYイベント処理
	void WndProc_WMDestroy()
	{
		//DWMain終了
		dw::DWMain::terminate();

		//DWWindow破棄
		dw::DWWindow::destroy();

		::PostQuitMessage(0);
	}

	//ウィンドウプロシージャ
	LRESULT CALLBACK WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
	{
		LRESULT ret = 0L;

		switch (msg) {
		case WM_CREATE:
			WndProc_WMCreate(hWnd);
			break;

		case WM_DESTROY:
			WndProc_WMDestroy();
			break;

		default:
			//デフォルト処理
			ret = ::DefWindowProc(hWnd, msg, wParam, lParam);
			break;
		}

		return ret;
	}
}

//メイン
int main()
{
	//コンソールウィンドウ生成
	::AllocConsole();
	FILE* fConsole = nullptr;
	freopen_s(&fConsole, "CONOUT$", "w", stdout);

	//インスタンスハンドル取得
	HINSTANCE hInstance = ::GetModuleHandle(nullptr);

	//クラス登録情報設定
	WNDCLASSEX wcex;
	wcex.cbSize = sizeof(WNDCLASSEX);
	wcex.style = CS_HREDRAW | CS_VREDRAW;
	wcex.lpfnWndProc = WndProc;
	wcex.lpszClassName = WIN_NAME;
	wcex.cbClsExtra = 0;
	wcex.cbWndExtra = 0;
	wcex.hInstance = hInstance;
	wcex.hIcon = ::LoadIcon(nullptr, IDI_APPLICATION);
	wcex.hCursor = ::LoadCursor(nullptr, IDC_ARROW);
	wcex.hbrBackground = nullptr;
	wcex.lpszMenuName = nullptr;
	wcex.hIconSm = ::LoadIcon(nullptr, IDI_APPLICATION);

	//クラス登録
	ATOM ret = ::RegisterClassEx(&wcex);
	if (ret == 0) {
		goto END;
	}

	//ウィンドウ作成
	HWND hWnd = ::CreateWindowEx(
		0,
		WIN_NAME,
		WIN_NAME,
		WS_OVERLAPPEDWINDOW,
		WIN_POSX,
		WIN_POSY,
		WIN_WIDTH,
		WIN_HEIGHT,
		nullptr,
		nullptr,
		hInstance,
		nullptr
	);
	if (hWnd == nullptr) {
		goto END;
	}

	// ウィンドウ表示
	ShowWindow(hWnd, SW_SHOW);
	UpdateWindow(hWnd);

	//メッセージループ
	MSG msg;
	while (GetMessage(&msg, nullptr, 0, 0) != 0) {
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}

END:
	//コンソールウィンドウ破棄
	fclose(fConsole);
	//::FreeConsole();	//x64で例外発生

	return 0;
}
