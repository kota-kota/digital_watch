#include "DWMain.hpp"

namespace {
	//DWMainインスタンス
	dw::DWMain* g_dwmain = nullptr;
	//グローバルミューテックス
	std::mutex g_mtx;
}

namespace dw {

	//----------------------------------------------------------------
	// DWMainクラス
	//----------------------------------------------------------------

	//開始
	void DWMain::start()
	{
		//ZDigitalWatchインスタンスが未生成なら生成する
		g_mtx.lock();
		if (g_dwmain == nullptr) {
			g_dwmain = new DWMain();
		}
		g_mtx.unlock();
	}

	//終了
	void DWMain::terminate()
	{
		g_mtx.lock();
		if (g_dwmain != nullptr) {
			delete g_dwmain;
			g_dwmain = nullptr;
		}
		g_mtx.unlock();
	}

	//コンストラクタ
	DWMain::DWMain() :
		th_(), mtx_(), taskState_(END)
	{
		//タスク開始
		this->mtx_.lock();
		this->taskState_ = START;
		this->mtx_.unlock();
		//スレッド作成
		this->th_ = std::thread(&DWMain::task, this);
	}

	//デストラクタ
	DWMain::~DWMain()
	{
		//タスク終了
		this->mtx_.lock();
		this->taskState_ = END;
		this->mtx_.unlock();
		//スレッド破棄
		this->th_.join();
	}

	//メインタスク
	void DWMain::task()
	{
		while (true) {
			//タスク状態を取得
			this->mtx_.lock();
			const bool endTask = (this->taskState_ == END) ? true : false;
			this->mtx_.unlock();

			if (endTask) {
				//タスク終了
				break;
			}

			//時刻取得
			const DWTime dwTime = DWFunc::getTime();

			//描画開始
			DWWindow* dwwin = DWWindow::get();
			dwwin->beginDraw();

			//画面クリア
			const DWColor color = { 255, 255, 255, 255 };
			dwwin->clear(color);

			//時刻数字描画
			DWText text = { 0 };
			text.textSize_ = 32;
			text.textNum_ = dwTime.strNum_;
			for (std::int32_t i = 0; i < dwTime.strNum_; i++) {
				text.text_[i] = static_cast<std::uint16_t>(dwTime.str_[i]);
			}
			DWCoord textCoord = { 0, 0 };
			DWColor textColor = { 0, 0, 255, 255 };
			dwwin->drawText(text, textCoord, textColor);

			//時刻画像描画
			DWCoord bitmapCoord = { 0, text.textSize_ };
			for (std::int32_t i = 0; i < dwTime.strNum_; i++) {
				DWImageFormat format;
				std::string filePath = DWFunc::getFilePath_TimeNumImage(dwTime.str_[i], format);
				DWImageDecorder decorder;
				decorder.decode_RGBA8888(filePath.c_str(), nullptr, format);
				DWBitmap bitmap;
				bitmap.image_ = decorder.getDecodeData(&bitmap.imageSize_, &bitmap.width_, &bitmap.height_);
				dwwin->drawBitmap(bitmap, bitmapCoord);

				bitmapCoord.x_ += bitmap.width_;
			}

			//描画終了
			dwwin->endDraw();

			//待ち
			std::this_thread::sleep_for(std::chrono::milliseconds(100));
		}
	}
}
