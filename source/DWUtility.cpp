#include "DWUtility.hpp"

namespace {
	//DWWindowインスタンス
	dw::DWWindow* g_dwwindow = nullptr;
	//グローバルミューテックス
	std::mutex g_mtx;

	//RGBA8888画像の1ピクセルあたりのバイト数
	static const std::int32_t BYTE_PER_PIXEL_RGBA8888 = 4;
}

namespace {

	//ByteReaderクラス
	class ByteReader {
	public:
		//1バイトを読み込み(LE)
		static void read1ByteLe(const std::uint8_t* const data, const std::int32_t offset, uint8_t* const readData)
		{
			*readData = *(data + offset + 0);
		}

		//2バイトを読み込み(LE)
		static void read2ByteLe(const std::uint8_t* const data, const std::int32_t offset, uint16_t* const readData)
		{
			*readData = 0;
			uint16_t v1 = uint16_t(*(data + offset + 0));
			*readData |= v1 << 0;
			uint16_t v2 = uint16_t(*(data + offset + 1));
			*readData |= v2 << 8;
		}

		//4バイトを読み込み(LE)
		static void read4ByteLe(const std::uint8_t* const data, const std::int32_t offset, uint32_t* const readData)
		{
			*readData = 0;
			uint32_t v1 = uint32_t(*(data + offset + 0));
			*readData |= v1 << 0;
			uint32_t v2 = uint32_t(*(data + offset + 1));
			*readData |= v2 << 8;
			uint32_t v3 = uint32_t(*(data + offset + 2));
			*readData |= v3 << 16;
			uint32_t v4 = uint32_t(*(data + offset + 3));
			*readData |= v4 << 24;
		}
	};
}

namespace dw {

	//----------------------------------------------------------------
	// DWWindowクラス
	//----------------------------------------------------------------

	//作成
	void DWWindow::create(void* native)
	{
		//Windowインスタンスが未生成なら生成する
		g_mtx.lock();
		if (g_dwwindow == nullptr) {
			g_dwwindow = new DWWindow(static_cast<HWND>(native));
		}
		g_mtx.unlock();
	}

	//取得
	DWWindow* DWWindow::get()
	{
		return g_dwwindow;
	}

	//破棄
	void DWWindow::destroy()
	{
		g_mtx.lock();
		if (g_dwwindow != nullptr) {
			delete g_dwwindow;
			g_dwwindow = nullptr;
		}
		g_mtx.unlock();
	}

	//描画開始
	void DWWindow::beginDraw()
	{
		if (this->hGLRC_ == nullptr) {
			//描画コンテキストハンドルを作成
			this->hGLRC_ = ::wglCreateContext(this->hDC_);

			//描画コンテキストをカレントに設定
			::wglMakeCurrent(this->hDC_, this->hGLRC_);
		}

		//ビューポート設定
		glViewport(0, 0, this->size_.width_, this->size_.height_);

		//プロジェクション設定(正射影)
		glMatrixMode(GL_PROJECTION);
		glLoadIdentity();
		glOrtho(0.0, this->size_.width_, this->size_.height_, 0.0, -1.0, 1.0);

		//モデルビュー設定
		glMatrixMode(GL_MODELVIEW);
		glLoadIdentity();
	}

	//描画終了
	void DWWindow::endDraw()
	{
		::SwapBuffers(this->hDC_);
	}

	//画面塗りつぶし
	void DWWindow::clear(const DWColor& color)
	{
		GLclampf r = static_cast<std::float32_t>(color.r_) / 255.0F;
		GLclampf g = static_cast<std::float32_t>(color.g_) / 255.0F;
		GLclampf b = static_cast<std::float32_t>(color.b_) / 255.0F;
		GLclampf a = static_cast<std::float32_t>(color.a_) / 255.0F;

		glClearColor(r, g, b, a);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	}

	//文字描画
	void DWWindow::drawText(const DWText& text, const DWCoord& coord, const DWColor& color)
	{
		//テキスト文字列のフォントグリフ保持用
		const std::int32_t MAX_GLYPHS = 32;
		FontGlyph glyphs[MAX_GLYPHS];
		std::int32_t numGlyphs = 0;

		//文字列のバウンディングボックス
		FT_BBox stringBBox;

		//フォントサイズ設定
		FT_Set_Char_Size(this->ftFace_, text.textSize_ * 64, 0, 96, 0);

		//テキスト文字分ループ
		DWCoord baseCoord = coord;
		for (std::int32_t i = 0; i < text.textNum_; i++) {
			//処理対象文字のグリフ格納用
			FontGlyph* glyph = &glyphs[numGlyphs];

			//グリフインデックスを取得
			glyph->index_ = FT_Get_Char_Index(this->ftFace_, text.text_[i]);

			//グリフをロード
			FT_Load_Glyph(this->ftFace_, glyph->index_, FT_LOAD_DEFAULT);

			//グリフを描画
			FT_Get_Glyph(this->ftFace_->glyph, &glyph->image_);
			FT_Glyph_To_Bitmap(&glyph->image_, FT_RENDER_MODE_NORMAL, nullptr, 1);

			//寸法情報を取得
			FT_BitmapGlyph bit = (FT_BitmapGlyph)glyph->image_;
			glyph->metrics_.width_ = bit->bitmap.width;
			glyph->metrics_.height_ = bit->bitmap.rows;
			glyph->metrics_.offsetX_ = bit->left;
			glyph->metrics_.offsetY_ = bit->top;
			glyph->metrics_.nextX_ = this->ftFace_->glyph->advance.x >> 6;
			glyph->metrics_.nextY_ = this->ftFace_->glyph->advance.y >> 6;

			//処理対象文字のバウンディングボックスを取得
			FT_BBox bbox;
			FT_Glyph_Get_CBox(glyph->image_, ft_glyph_bbox_pixels, &bbox);

			if (i == 0) {
				stringBBox.xMin = 0;
				stringBBox.xMax = glyph->metrics_.nextX_;
				stringBBox.yMin = bbox.yMin;
				stringBBox.yMax = bbox.yMax;
			}
			else {
				stringBBox.xMin = 0;
				stringBBox.xMax += glyph->metrics_.nextX_;
				if (bbox.yMin < stringBBox.yMin) { stringBBox.yMin = bbox.yMin; }
				if (bbox.yMax > stringBBox.yMax) { stringBBox.yMax = bbox.yMax; }
			}

			//次文字へ
			numGlyphs++;
		}

		//文字列の幅高さ
		const std::int32_t stringW = stringBBox.xMax - stringBBox.xMin;
		const std::int32_t stringH = stringBBox.yMax - stringBBox.yMin;
		const std::int32_t stringSize = stringW * stringH;

		//文字列画像の領域を確保
		std::uint8_t* tex = new std::uint8_t[stringSize];
		memset(tex, 0, stringSize);

		//文字列画像を生成
		FT_Vector pen = { 0, stringBBox.yMax };
		for (std::int32_t i = 0; i < numGlyphs; i++) {
			//処理対象文字のグリフを取得
			FontGlyph* glyph = &glyphs[i];
			FT_BitmapGlyph bit = (FT_BitmapGlyph)glyph->image_;

			const std::int32_t xoffset = pen.x + glyph->metrics_.offsetX_;
			const std::int32_t yoffset = pen.y - glyph->metrics_.offsetY_;
			std::int32_t readOffset = 0;
			std::int32_t writeOffset = xoffset + (yoffset * stringW);
			for (std::int32_t h = 0; h < glyph->metrics_.height_; h++) {
				memcpy_s(tex + writeOffset, stringW, bit->bitmap.buffer + readOffset, glyph->metrics_.width_);
				readOffset += glyph->metrics_.width_;
				writeOffset += stringW;
			}

			pen.x += glyph->metrics_.nextX_;

			//グリフイメージ破棄
			FT_Done_Glyph(glyph->image_);
		}

		//GL描画設定
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		glEnable(GL_BLEND);
		glEnable(GL_TEXTURE_2D);

		//テクスチャ生成
		GLuint texID;
		glGenTextures(1, &texID);

		//テクスチャバインド
		glBindTexture(GL_TEXTURE_2D, texID);

		//テクスチャロード(ロード後、文字列画像の領域を解放)
		glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_ALPHA, stringW, stringH, 0, GL_ALPHA, GL_UNSIGNED_BYTE, tex);
		delete[] tex;

		//テクスチャパラメータ設定
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);

		DWArea drawArea;
		drawArea.xmin_ = baseCoord.x_;
		drawArea.ymin_ = baseCoord.y_;
		drawArea.xmax_ = baseCoord.x_ + stringW;
		drawArea.ymax_ = baseCoord.y_ + stringH;;

		//描画
		glColor4ub(color.r_, color.g_, color.b_, color.a_);
		glBegin(GL_TRIANGLE_STRIP);
		glTexCoord2f(0.0F, 0.0F);
		glVertex2i(drawArea.xmin_, drawArea.ymin_);
		glTexCoord2f(1.0F, 0.0F);
		glVertex2i(drawArea.xmax_, drawArea.ymin_);
		glTexCoord2f(0.0F, 1.0F);
		glVertex2i(drawArea.xmin_, drawArea.ymax_);
		glTexCoord2f(1.0F, 1.0F);
		glVertex2i(drawArea.xmax_, drawArea.ymax_);
		glEnd();

		//テクスチャアンバインド
		glBindTexture(GL_TEXTURE_2D, 0);
		//テクスチャ破棄
		glDeleteTextures(1, &texID);

		glDisable(GL_TEXTURE_2D);
		glDisable(GL_BLEND);
	}

	//画像描画
	void DWWindow::drawBitmap(const DWBitmap& bitmap, const DWCoord& coord)
	{
		//GL描画設定
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		glEnable(GL_BLEND);
		glEnable(GL_TEXTURE_2D);

		//テクスチャ生成
		GLuint texID;
		glGenTextures(1, &texID);

		//テクスチャバインド
		glBindTexture(GL_TEXTURE_2D, texID);

		//テクスチャロード
		glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, bitmap.width_, bitmap.height_, 0, GL_RGBA, GL_UNSIGNED_BYTE, bitmap.image_);

		//テクスチャパラメータ設定
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);

		//テクスチャ環境
		//テクスチャカラーを使用する
		glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);

		DWArea drawArea;
		drawArea.xmin_ = coord.x_;
		drawArea.ymin_ = coord.y_;
		drawArea.xmax_ = coord.x_ + bitmap.width_;
		drawArea.ymax_ = coord.y_ + bitmap.height_;

		//描画
		glColor4ub(255, 255, 255, 255);
		glBegin(GL_TRIANGLE_STRIP);
		glTexCoord2f(0.0F, 0.0F);
		glVertex2i(drawArea.xmin_, drawArea.ymin_);
		glTexCoord2f(1.0F, 0.0F);
		glVertex2i(drawArea.xmax_, drawArea.ymin_);
		glTexCoord2f(0.0F, 1.0F);
		glVertex2i(drawArea.xmin_, drawArea.ymax_);
		glTexCoord2f(1.0F, 1.0F);
		glVertex2i(drawArea.xmax_, drawArea.ymax_);
		glEnd();

		//テクスチャアンバインド
		glBindTexture(GL_TEXTURE_2D, 0);
		//テクスチャ破棄
		glDeleteTextures(1, &texID);

		glDisable(GL_TEXTURE_2D);
		glDisable(GL_BLEND);
	}

	//コンストラクタ
	DWWindow::DWWindow(HWND hWnd) :
		hWnd_(hWnd), hDC_(nullptr), hGLRC_(nullptr), ftLibrary_(nullptr), ftFace_(nullptr), size_()
	{
		//デバイスコンテキストハンドルを取得
		this->hDC_ = ::GetDC(this->hWnd_);

		//ピクセルフォーマット
		const PIXELFORMATDESCRIPTOR pFormat = {
			sizeof(PIXELFORMATDESCRIPTOR),	//nSize
			1,		//nVersion
			PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER,	//dwFlags
			PFD_TYPE_RGBA,	//iPixelType
			32,		//cColorBits
			0,		//cRedBits
			0,		//cRedShift
			0,		//cGreenBits
			0,		//cGreenShift
			0,		//cBlueBits
			0,		//cBlueShift
			0,		//cAlphaBits
			0,		//cAlphaShift
			0,		//cAccumBits
			0,		//cAccumRedBits
			0,		//cAccumGreenBits
			0,		//cAccumBlueBits
			0,		//cAccumAlphaBits
			24,		//cDepthBits
			8,		//cStencilBits
			0,		//cAuxBuffers
			0,		//iLayerType
			0,		//bReserved
			0,		//dwLayerMask
			0,		//dwVisibleMask
			0		//dwDamageMask
		};

		//ピクセルフォーマットを選択
		std::int32_t format = ::ChoosePixelFormat(this->hDC_, &pFormat);
		::SetPixelFormat(this->hDC_, format, &pFormat);

		//FreeType開始
		FT_Init_FreeType(&this->ftLibrary_);
		//MeiryoUIフォントのフェイスを生成
		FT_New_Face(this->ftLibrary_, "C:/Windows/Fonts/meiryo.ttc", 0, &this->ftFace_);

		//ウィンドウサイズを取得
		RECT rect;
		::GetClientRect(hWnd, &rect);
		this->size_.width_ = rect.right - rect.left;
		this->size_.height_ = rect.bottom - rect.top;
	}

	//デストラクタ
	DWWindow::~DWWindow()
	{
		//フェイスを破棄
		FT_Done_Face(this->ftFace_);
		//FreeType終了
		FT_Done_FreeType(this->ftLibrary_);

		//カレントを解除
		::wglMakeCurrent(this->hDC_, nullptr);

		if (this->hGLRC_ != nullptr) {
			//描画コンテキストハンドルを破棄
			::wglDeleteContext(this->hGLRC_);
		}

		if (this->hDC_ != nullptr) {
			//デバイスコンテキストを破棄
			::ReleaseDC(this->hWnd_, this->hDC_);
		}
	}


	//----------------------------------------------------------------
	// DWImageDecorderクラス
	//----------------------------------------------------------------

	//コンストラクタ
	DWImageDecorder::DWImageDecorder() :
		decData_(nullptr), decDataSize_(0), width_(0), height_(0)
	{
	}

	//デストラクタ
	DWImageDecorder::~DWImageDecorder()
	{
		if (this->decData_ != nullptr) {
			delete[] this->decData_;
		}
	}

	//RGBA8888画像へデコード
	std::int32_t DWImageDecorder::decode_RGBA8888(const std::char8_t* const bodyFilePath, const std::char8_t* const blendFilePath, const DWImageFormat format, const bool isFlip)
	{
		std::int32_t rc = -1;

		//画像データ
		std::uint8_t* bodyData = nullptr;
		std::uint8_t* blendData = nullptr;
		std::int32_t bodyDataSize = 0;
		std::int32_t blendDataSize = 0;

		//本体画像ファイルオープン
		std::ifstream ifs_body(bodyFilePath, std::ios::binary);
		if (!ifs_body) {
			//オープン失敗
			goto END;
		}

		//本体画像データサイズ取得
		ifs_body.seekg(0, std::ios::end);
		bodyDataSize = static_cast<std::int32_t>(ifs_body.tellg());
		ifs_body.seekg(0, std::ios::beg);
		if (bodyDataSize <= 0) {
			//サイズ異常
			goto END;
		}

		//本体画像データ領域を確保し、ファイル読み込み
		bodyData = new std::uint8_t[bodyDataSize];
		ifs_body.read(reinterpret_cast<std::char8_t*>(bodyData), bodyDataSize);

		//ブレンド画像ファイルの指定があれば、ブレンド画像を読み込む
		if (blendFilePath != nullptr) {
			//ブレンド画像ファイルオープン
			std::ifstream ifs_blend(blendFilePath, std::ios::binary);
			if (!ifs_blend) {
				//オープン失敗
				goto END;
			}

			//ブレンド画像データサイズ取得
			ifs_blend.seekg(0, std::ios::end);
			blendDataSize = static_cast<std::int32_t>(ifs_blend.tellg());
			ifs_blend.seekg(0, std::ios::beg);
			if (blendDataSize <= 0) {
				//サイズ異常
				goto END;
			}

			//ブレンド画像データ領域を確保し、ファイル読み込み
			blendData = new std::uint8_t[blendDataSize];
			ifs_blend.read(reinterpret_cast<std::char8_t*>(blendData), blendDataSize);
		}

		//RGBA8888画像へデコード
		rc = this->decode_RGBA8888(bodyData, bodyDataSize, blendData, blendDataSize, format, isFlip);

	END:
		//画像データ読み込み用領域を解放
		if (bodyData != nullptr) {
			delete[] bodyData;
		}
		if (blendData != nullptr) {
			delete[] blendData;
		}
		return rc;
	}
	std::int32_t DWImageDecorder::decode_RGBA8888(std::uint8_t* const bodyData, const std::int32_t bodyDataSize, std::uint8_t* const blendData, const std::int32_t blendDataSize, const DWImageFormat format, const bool isFlip)
	{
		std::int32_t rc = -1;
		std::int32_t ret = -1;

		//以前のデコードデータがあれば解放
		if (this->decData_ != nullptr) {
			delete[] this->decData_;
		}

		//画像フォーマット毎の処理
		if (format == BMP) {
			//BMP画像

			//本体画像をデコード
			ret = this->decodeBMP_RGBA8888(bodyData, bodyDataSize);
			if (ret < 0) {
				goto END;
			}

			//ブレンド画像の指定があれば、ブレンド処理を実施
			if (blendData != nullptr) {
				ret = this->blendBMP_RGBA8888(blendData, blendDataSize);
				if (ret < 0) {
					goto END;
				}
			}
		}
		else if (format == PNG) {
			//PNG画像

			//本体画像をデコード
			ret = this->decodePNG_RGBA8888(bodyData, bodyDataSize);
			if (ret < 0) {
				goto END;
			}

			//ブレンド画像の指定があれば、ブレンド処理を実施
			if (blendData != nullptr) {
				ret = this->blendPNG_RGBA8888(blendData, blendDataSize);
				if (ret < 0) {
					goto END;
				}
			}
		}
		else {
		}

		if (isFlip) {
			//デコード結果を上下反転する
		}

		//正常終了
		rc = 0;

	END:
		return rc;
	}

	//デコードデータ取得
	std::uint8_t* DWImageDecorder::getDecodeData(std::int32_t* const decDataSize, std::int32_t* const width, std::int32_t* const height)
	{
		if (decDataSize != nullptr) { *decDataSize = this->decDataSize_; }
		if (width != nullptr) { *width = this->width_; }
		if (height != nullptr) { *height = this->height_; }
		return this->decData_;
	}

	//本体BMP画像をRGBA8888画像へデコード
	std::int32_t DWImageDecorder::decodeBMP_RGBA8888(std::uint8_t* const bodyData, const std::int32_t bodyDataSize)
	{
		std::int32_t rc = -1;
		std::int32_t ret = -1;

		//本体BMP画像オブジェクト生成
		DWImageBMP bmp_body;
		ret = bmp_body.create(bodyData, bodyDataSize);
		if (ret < 0) {
			//生成失敗
			goto END;
		}

		//本体BMP画像幅高さを取得
		bmp_body.getWH(&this->width_, &this->height_);

		//デコードデータ格納領域を確保
		this->decDataSize_ = this->width_ * this->height_ * BYTE_PER_PIXEL_RGBA8888;
		this->decData_ = new std::uint8_t[this->decDataSize_];

		//本体BMP画像をRGBA8888画像へデコード
		ret = bmp_body.decode_RGBA8888(&this->decData_);
		if (ret < 0) {
			//デコード失敗
			goto END;
		}

		//正常終了
		rc = 0;

	END:
		return rc;
	}

	//ブレンドBMP画像をRGBA8888画像へデコードし、本体デコード画像へブレンド
	std::int32_t DWImageDecorder::blendBMP_RGBA8888(std::uint8_t* const blendData, const std::int32_t blendDataSize)
	{
		std::int32_t rc = -1;
		std::int32_t ret = -1;

		std::uint8_t* decData_blend = nullptr;

		//ブレンドBMP画像オブジェクト生成
		DWImageBMP bmp_blend;
		ret = bmp_blend.create(blendData, blendDataSize);
		if (ret < 0) {
			//生成失敗
			goto END;
		}

		//ブレンドBMP画像幅高さを取得
		std::int32_t width_blend, height_blend;
		bmp_blend.getWH(&width_blend, &height_blend);
		if ((this->width_ != width_blend) || (this->height_ != height_blend)) {
			//幅高さが不一致
			goto END;
		}

		//デコードデータ格納領域を確保
		decData_blend = new std::uint8_t[this->decDataSize_];

		//ブレンドBMP画像をRGBA8888画像へデコード
		ret = bmp_blend.decode_RGBA8888(&decData_blend);
		if (ret < 0) {
			//デコード失敗
			goto END;
		}

		//RGBA8888画像のブレンド処理
		this->blend_RGBA8888(decData_blend);

		//正常終了
		rc = 0;

	END:
		if (decData_blend != nullptr) {
			delete[] decData_blend;
		}
		return rc;
	}

	//本体PNG画像をRGBA8888画像へデコード
	std::int32_t DWImageDecorder::decodePNG_RGBA8888(std::uint8_t* const bodyData, const std::int32_t bodyDataSize)
	{
		std::int32_t rc = -1;
		std::int32_t ret = -1;

		//本体PNG画像オブジェクト生成
		DWImagePNG png_body;
		ret = png_body.create(bodyData, bodyDataSize);
		if (ret < 0) {
			//生成失敗
			goto END;
		}

		//本体PNG画像幅高さを取得
		png_body.getWH(&this->width_, &this->height_);

		//デコードデータ格納領域を確保
		this->decDataSize_ = this->width_ * this->height_ * BYTE_PER_PIXEL_RGBA8888;
		this->decData_ = new std::uint8_t[this->decDataSize_];

		//本体PNG画像をRGBA8888画像へデコード
		ret = png_body.decode_RGBA8888(&this->decData_);
		if (ret < 0) {
			//デコード失敗
			goto END;
		}

		//正常終了
		rc = 0;

	END:
		return rc;
	}

	//ブレンドPNG画像をRGBA8888画像へデコードし、本体デコード画像へブレンド
	std::int32_t DWImageDecorder::blendPNG_RGBA8888(std::uint8_t* const blendData, const std::int32_t blendDataSize)
	{
		std::int32_t rc = -1;
		std::int32_t ret = -1;

		std::uint8_t* decData_blend = nullptr;

		//ブレンドPNG画像オブジェクト生成
		DWImagePNG png_blend;
		ret = png_blend.create(blendData, blendDataSize);
		if (ret < 0) {
			//生成失敗
			goto END;
		}

		//ブレンドPNG画像幅高さを取得
		std::int32_t width_blend, height_blend;
		png_blend.getWH(&width_blend, &height_blend);
		if ((this->width_ != width_blend) || (this->height_ != height_blend)) {
			//幅高さが不一致
			goto END;
		}

		//デコードデータ格納領域を確保
		decData_blend = new std::uint8_t[this->decDataSize_];

		//ブレンドPNG画像をRGBA8888画像へデコード
		ret = png_blend.decode_RGBA8888(&decData_blend);
		if (ret < 0) {
			//デコード失敗
			goto END;
		}

		//RGBA8888画像のブレンド処理
		this->blend_RGBA8888(decData_blend);

		//正常終了
		rc = 0;

	END:
		if (decData_blend != nullptr) {
			delete[] decData_blend;
		}
		return rc;
	}


	//RGBA8888画像のブレンド処理
	void DWImageDecorder::blend_RGBA8888(std::uint8_t* const decData_blend)
	{
		std::int32_t offset = 0;
		for (std::int32_t h = 0; h < this->height_; h++) {
			for (std::int32_t w = 0; w < this->width_; w++) {
				this->decData_[offset + 3] = decData_blend[offset];
				offset += BYTE_PER_PIXEL_RGBA8888;
			}
		}
	}




	//----------------------------------------------------------------
	// DWImageBMPクラス
	//----------------------------------------------------------------

	//コンストラクタ
	DWImageBMP::DWImageBMP() :
		bmp_(nullptr), bmpSize_(0), format_(BitmapFormat::INVALID), fileSize_(0), imageOffset_(0),
		width_(0), height_(0), bitCount_(0), compression_(0), imageSize_(0), palleteNum_(0), palleteByte_(0), palleteOffset_(0)
	{
	}

	//デストラクタ
	DWImageBMP::~DWImageBMP()
	{
	}

	//作成
	std::int32_t DWImageBMP::create(std::uint8_t* const bmp, const std::int32_t bmpSize)
	{
		std::int32_t rc = 0;

		//メンバへ保持
		this->bmp_ = bmp;
		this->bmpSize_ = bmpSize;

		//------------------------------------------
		//ファイルヘッダ読み込み

		//ファイルタイプを取得
		std::uint8_t fileType[2];
		ByteReader::read1ByteLe(this->bmp_, BFH_FILETYPE_OFS, &fileType[0]);
		ByteReader::read1ByteLe(this->bmp_, BFH_FILETYPE_OFS + 1, &fileType[1]);
		if ((fileType[0] == 'B') && (fileType[1] == 'M')) {
			//Bitmap画像

			//ファイルサイズを取得
			std::uint32_t fileSize = 0;
			ByteReader::read4ByteLe(this->bmp_, BFH_FILESIZE_OFS, &fileSize);
			this->fileSize_ = fileSize;

			//ファイル先頭から画像データまでのオフセットを取得
			std::uint32_t imageOffset = 0;
			ByteReader::read4ByteLe(this->bmp_, BFH_IMAGEOFS_OFS, &imageOffset);
			this->imageOffset_ = imageOffset;

			//情報ヘッダサイズを取得
			std::uint32_t infoHeaderSize = 0;
			ByteReader::read4ByteLe(this->bmp_, BIH_HEADERSIZE_OFS, &infoHeaderSize);

			//情報ヘッダサイズに応じてフォーマットを選択
			if (infoHeaderSize == BIH_HEADERSIZE) {
				//Windowsフォーマット
				this->format_ = BitmapFormat::WIN;

				//Bitmap情報ヘッダ(Windows)読み込み
				rc = this->readInfoHeader_WINDOWS();
			}
			else if (infoHeaderSize == BCH_HEADERSIZE) {
				//OS/2フォーマット
				this->format_ = BitmapFormat::OS2;

				//Bitmap情報ヘッダ(OS/2)読み込み
				rc = this->readInfoHeader_OS2();
			}
			else {
				//異常
				rc = -1;
			}
		}
		else {
			//Bitmap画像でない
			rc = -1;
		}

		return rc;
	}

	//幅高さ取得
	void DWImageBMP::getWH(std::int32_t* const width, std::int32_t* const height)
	{
		if (width != nullptr) { *width = this->width_; }
		if (height != nullptr) { *height = this->height_; }
	}

	//RGBA8888画像へデコード
	std::int32_t DWImageBMP::decode_RGBA8888(std::uint8_t** const decData)
	{
		std::int32_t rc = 0;

		switch (this->bitCount_) {
		case 1:		//1bit
		case 4:		//4bit
		case 8:		//8bit
			rc = this->decodePalleteBitmap_RGBA8888(decData);
			break;
		case 24:	//24bit
		case 32:	//32bit
			rc = this->decodeTrueColorBitmap_RGBA8888(decData);
			break;
		default:
			rc = -1;
			break;
		}

		return rc;
	}

	//Bitmap情報ヘッダ(Windows)読み込み
	std::int32_t DWImageBMP::readInfoHeader_WINDOWS()
	{
		//画像の幅と高さを取得
		std::uint32_t width = 0;
		std::uint32_t height = 0;
		ByteReader::read4ByteLe(this->bmp_, BIH_WIDTH_OFS, &width);
		ByteReader::read4ByteLe(this->bmp_, BIH_HEIGHT_OFS, &height);

		//色ビット数を取得
		std::uint16_t bitCount = 0;
		ByteReader::read2ByteLe(this->bmp_, BIH_BITCOUNT_OFS, &bitCount);

		//圧縮形式を取得
		std::uint32_t compression = 0;
		ByteReader::read4ByteLe(this->bmp_, BIH_COMPRESSION_OFS, &compression);

		//画像データサイズを取得
		std::uint32_t imageSize = 0;
		ByteReader::read4ByteLe(this->bmp_, BIH_IMGDATASIZE_OFS, &imageSize);

		//パレット数を取得
		std::uint32_t palleteNum = 0;
		ByteReader::read4ByteLe(this->bmp_, BIH_PALLETENUM_OFS, &palleteNum);
		if ((palleteNum == 0) && (bitCount <= 8)) {
			//パレット数が0かつビット数が8以下の場合は、ビット数からパレット数を計算
			palleteNum = 1 << bitCount;
		}

		//メンバ変数に設定
		this->width_ = int32_t(width);
		this->height_ = int32_t(height);
		this->bitCount_ = int32_t(bitCount);
		this->compression_ = int32_t(compression);
		this->imageSize_ = int32_t(imageSize);
		this->palleteNum_ = int32_t(palleteNum);
		this->palleteByte_ = int32_t(4);
		this->palleteOffset_ = uint32_t(BIH_PALLETE_OFS);

		return 0;
	}

	//Bitmap情報ヘッダ(OS/2)読み込み
	std::int32_t DWImageBMP::readInfoHeader_OS2()
	{
		//画像の幅と高さを取得
		std::uint16_t width = 0;
		std::uint16_t height = 0;
		ByteReader::read2ByteLe(this->bmp_, BCH_WIDTH_OFS, &width);
		ByteReader::read2ByteLe(this->bmp_, BCH_HEIGHT_OFS, &height);

		//色ビット数を取得
		std::uint16_t bitCount = 0;
		ByteReader::read2ByteLe(this->bmp_, BCH_BITCOUNT_OFS, &bitCount);

		//パレット数を取得
		std::uint32_t palleteNum = 0;
		if (bitCount <= 8) {
			//ビット数が8以下の場合は、パレット数を計算
			palleteNum = 1 << bitCount;
		}

		//メンバ変数に設定
		this->width_ = int32_t(width);
		this->height_ = int32_t(height);
		this->bitCount_ = int32_t(bitCount);
		this->compression_ = int32_t(0);
		this->imageSize_ = int32_t(0);
		this->palleteNum_ = int32_t(palleteNum);
		this->palleteByte_ = int32_t(3);
		this->palleteOffset_ = int32_t(BCH_PALLETE_OFS);

		return 0;
	}

	//パディングバイト数を取得
	std::uint32_t DWImageBMP::getPaddingByte()
	{
		//画像データサイズ = ((((色ビット数 * 画像の幅) + パティング) × 画像の高さ) / 8)から計算

		//画像データサイズを取得
		std::uint32_t imageSize = this->imageSize_;
		if (imageSize == 0) {
			imageSize = this->fileSize_ - this->imageOffset_;
		}

		//パディングビット数を計算
		std::uint32_t paddingBit = ((imageSize * 8) / this->height_) - (this->width_ * this->bitCount_);

		//パディングバイト数を返す
		return paddingBit / 8;
	}

	//パレットデータを取得
	std::int32_t DWImageBMP::getPalleteData(PalColor* const pallete, const std::int32_t numMaxPal)
	{
		std::int32_t rc = 0;

		if (this->palleteNum_ <= numMaxPal) {
			//パレットデータを取得
			std::uint32_t readOffset = this->palleteOffset_;
			std::uint32_t writeOffset = 0;
			for (std::uint16_t p = 0; p < this->palleteNum_; p++) {
				//青→緑→赤
				ByteReader::read1ByteLe(this->bmp_, int32_t(readOffset) + 0, &(pallete[writeOffset].b_));
				ByteReader::read1ByteLe(this->bmp_, int32_t(readOffset) + 1, &(pallete[writeOffset].g_));
				ByteReader::read1ByteLe(this->bmp_, int32_t(readOffset) + 2, &(pallete[writeOffset].r_));

				writeOffset++;
				readOffset += 3;
				if (this->palleteByte_ == 4) {
					//palleteByte_が4の場合、予約領域分を読み飛ばし
					readOffset++;
				}
			}
		}
		else {
			//パレット数異常
			rc = -1;
		}

		return rc;
	}

	//パレットBMP画像からRGBA8888画像へデコード
	std::int32_t DWImageBMP::decodePalleteBitmap_RGBA8888(std::uint8_t** const decData)
	{
		std::int32_t rc = 0;

		//パレットデータを取得
		PalColor pallete[PALLETE_MAXNUM] = { 0 };
		rc = this->getPalleteData(pallete, PALLETE_MAXNUM);
		if (rc == 0) {

			//パディングバイト数を取得
			std::int32_t paddingByte = this->getPaddingByte();

			//出力データへデコード後の画像データを設定
			std::int32_t readOffset = this->imageOffset_;
			for (std::int32_t h = 0; h < this->height_; h++) {
				//一行ずつ処理
				std::int32_t writeOffset = (this->height_ - h - 1) * this->width_ * BYTE_PER_PIXEL_RGBA8888;

				//ビット数に応じたビットオフセットとビットマスク
				std::int32_t bitOfs = 8 - this->bitCount_;
				std::uint8_t bitMask = (0x01 << this->bitCount_) - 1;

				for (std::int32_t w = 0; w < this->width_; w++) {
					//画像データはパレットインデックス
					std::uint8_t index = 0;
					ByteReader::read1ByteLe(this->bmp_, int16_t(readOffset), &index);
					std::int32_t palleteOffset = (index >> bitOfs) & bitMask;

					//出力データへRGBA値を設定
					*((*decData) + writeOffset + 0) = pallete[palleteOffset].r_;		//赤
					*((*decData) + writeOffset + 1) = pallete[palleteOffset].g_;		//緑
					*((*decData) + writeOffset + 2) = pallete[palleteOffset].b_;		//青
					*((*decData) + writeOffset + 3) = 255;

					//ビットオフセットを更新
					bitOfs -= this->bitCount_;
					if (bitOfs < 0) {
						//次の読み込み位置に更新
						bitOfs = 8 - this->bitCount_;
						readOffset++;
					}
					//書き込み位置を更新
					writeOffset += BYTE_PER_PIXEL_RGBA8888;
				}
				readOffset += paddingByte;
			}
		}
		else {
			//パレット取得失敗
			rc = -1;
		}

		return rc;
	}

	//トゥルーカラーBitmap画像からRGBA8888画像へデコード
	std::int32_t DWImageBMP::decodeTrueColorBitmap_RGBA8888(std::uint8_t** const decData)
	{
		//パディングバイト数を取得
		std::int32_t paddingByte = getPaddingByte();

		//出力データへデコード後の画像データを設定
		std::int32_t readOffset = this->imageOffset_;
		for (std::int32_t h = 0; h < this->height_; h++) {
			//一行ずつ処理
			std::int32_t writeIndex = (this->height_ - h - 1) * this->width_ * BYTE_PER_PIXEL_RGBA8888;

			for (std::int32_t w = 0; w < this->width_; w++) {
				//画像データはBGR値
				std::uint8_t b = 255;
				ByteReader::read1ByteLe(this->bmp_, int32_t(readOffset) + 0, &b);
				std::uint8_t g = 255;
				ByteReader::read1ByteLe(this->bmp_, int32_t(readOffset) + 1, &g);
				std::uint8_t r = 255;
				ByteReader::read1ByteLe(this->bmp_, int32_t(readOffset) + 2, &r);
				std::uint8_t a = 255;
				if (this->bitCount_ == 32) {
					//ビット数が32bitの場合、A値を読み込み
					ByteReader::read1ByteLe(this->bmp_, int32_t(readOffset) + 3, &a);

					//読み込み位置を更新
					readOffset++;
				}

				//出力データへRGBA値を設定
				*((*decData) + writeIndex + 0) = r;		//赤
				*((*decData) + writeIndex + 1) = g;		//緑
				*((*decData) + writeIndex + 2) = b;		//青
				*((*decData) + writeIndex + 3) = a;		//アルファ

														//読み込み位置を更新
				readOffset += 3;

				//書き込み位置を更新
				writeIndex += BYTE_PER_PIXEL_RGBA8888;
			}
			readOffset += paddingByte;
		}

		return 0;
	}



	//----------------------------------------------------------------
	// DWImagePNGクラス
	//----------------------------------------------------------------

	//コンストラクタ
	DWImagePNG::DWImagePNG() :
		png_(nullptr), pngSize_(0), width_(0), height_(0), rowByte_(0), bitDepth_(0), colorType_(0),
		pngStr_(nullptr), pngInfo_(nullptr)
	{
	}

	//デストラクタ
	DWImagePNG::~DWImagePNG()
	{
		if (this->pngInfo_ != nullptr) {
			png_destroy_info_struct(this->pngStr_, &this->pngInfo_);
		}
		if (this->pngStr_ != nullptr) {
			png_destroy_read_struct(&this->pngStr_, nullptr, nullptr);
		}
	}

	//PNG読み込みコールバック関数
	void DWImagePNG::callbackReadPng(png_structp pngStr, png_bytep data, png_size_t length)
	{
		png_bytepp buf = (png_bytepp)png_get_io_ptr(pngStr);
		memcpy_s(data, length, *buf, length);
		*buf += length;
	}

	//作成
	std::int32_t DWImagePNG::create(std::uint8_t* const png, const std::int32_t pngSize)
	{
		std::int32_t rc = 0;

		//メンバへ保持
		this->png_ = png;
		this->pngSize_ = pngSize;

		//PNGシグネチャのチェック
		png_byte sig[PNG_BYTES_TO_CHECK];
		(void)memcpy_s(sig, PNG_BYTES_TO_CHECK, this->png_, PNG_BYTES_TO_CHECK);
		if (png_sig_cmp(sig, 0, PNG_BYTES_TO_CHECK) == 0) {
			//PNG画像

			//PNG構造ポインタ作成
			this->pngStr_ = png_create_read_struct(PNG_LIBPNG_VER_STRING, nullptr, nullptr, nullptr);

			//PNG情報ポインタ作成
			this->pngInfo_ = png_create_info_struct(this->pngStr_);

			//シグネチャ読み込み済み
			png_set_sig_bytes(this->pngStr_, PNG_BYTES_TO_CHECK);
			this->png_ += PNG_BYTES_TO_CHECK;

			//PNG読み込みコールバック関数を登録
			png_set_read_fn(this->pngStr_, &this->png_, callbackReadPng);

			//PNG読み込み
			png_read_info(this->pngStr_, this->pngInfo_);

			//IHDRチャンクの各種情報取得
			this->width_ = png_get_image_width(this->pngStr_, this->pngInfo_);
			this->height_ = png_get_image_height(this->pngStr_, this->pngInfo_);
			this->rowByte_ = int32_t(png_get_rowbytes(this->pngStr_, this->pngInfo_));
			this->bitDepth_ = png_get_bit_depth(this->pngStr_, this->pngInfo_);
			this->colorType_ = png_get_color_type(this->pngStr_, this->pngInfo_);
		}
		else {
			//PNG画像でない
			rc = -1;
		}

		return rc;
	}

	//幅高さ取得
	void DWImagePNG::getWH(std::int32_t* const width, std::int32_t* const height)
	{
		if (width != nullptr) { *width = this->width_; }
		if (height != nullptr) { *height = this->height_; }
	}

	//RGBA8888画像へデコード
	std::int32_t DWImagePNG::decode_RGBA8888(std::uint8_t** const decData)
	{
		std::int32_t rc = 0;

		if (this->pngStr_ != nullptr) {
			//デコード後の画像データを格納するメモリを確保(本関数の最後に解放)
			png_size_t pngSize = this->height_ * sizeof(png_bytep) + this->height_ * this->rowByte_;
			png_byte* tmp = new png_byte[pngSize];
			png_bytepp png = (png_bytepp)tmp;

			//png_read_imageには各行へのポインタを渡すため、2次元配列化
			png_bytep wp = (png_bytep)&png[this->height_];
			for (std::int32_t h = 0; h < this->height_; h++) {
				png[h] = wp;
				wp += this->rowByte_;
			}

			//PNGイメージ読み込み
			png_read_image(this->pngStr_, png);

			switch (this->colorType_) {
			case PNG_COLOR_TYPE_GRAY:		//0:グレー
				rc = this->decodeGrayScalePng_RGBA8888(decData, png);
				break;
			case PNG_COLOR_TYPE_RGB:		//2:トゥルーカラー
				rc = this->decodeTrueColorPng_RGBA8888(decData, png, false);
				break;
			case PNG_COLOR_TYPE_PALETTE:	//3:パレット
				rc = this->decodePalletePng_RGBA8888(decData, png);
				break;
			case PNG_COLOR_TYPE_GRAY_ALPHA:	//4:グレー+アルファ
				break;
			case PNG_COLOR_TYPE_RGB_ALPHA:	//6:トゥルーカラー+アルファ
				rc = this->decodeTrueColorPng_RGBA8888(decData, png, true);
				break;
			default:
				break;
			};

			//デコード後の画像データを格納するメモリを解放
			delete[] png;
		}
		else {
			//PNG構造未作成
			rc = -1;
		}

		return rc;
	}

	//グレーPNG画像からRGBA8888画像へデコード
	std::int32_t DWImagePNG::decodeGrayScalePng_RGBA8888(std::uint8_t** const decData, const png_bytepp png)
	{
		std::int32_t rc = 0;

		if (this->bitDepth_ <= 8) {
			//ビット深度が1bit,2bit,4bit,8bitの場合

			//ビット深度で表現できる最大値
			png_byte bitMaxValue = (0x01 << this->bitDepth_) - 1;

			//グレーサンプル値(輝度に応じたグレーカラー取得に必要)
			png_byte graySample = 255 / bitMaxValue;

			//出力データへデコード後の画像データを設定
			for (std::int32_t h = 0; h < this->height_; h++) {
				//一行ずつ処理
				png_bytep wp = png[h];

				//書き込み位置と読み込み位置を初期化
				std::int32_t writeOffset = h * this->width_ * BYTE_PER_PIXEL_RGBA8888;
				std::int32_t readOffset = 0;

				//ビット深度に応じたビットオフセット
				png_int_16 bitOfs = 8 - this->bitDepth_;

				for (std::int32_t w = 0; w < this->width_; w++) {
					//輝度を取得
					png_byte brightness = (wp[readOffset] >> bitOfs) & bitMaxValue;

					//輝度に応じたグレーカラーを取得
					png_byte grayColor = graySample * brightness;

					//出力データへRGBA値を設定
					*((*decData) + writeOffset + 0) = grayColor;
					*((*decData) + writeOffset + 1) = grayColor;
					*((*decData) + writeOffset + 2) = grayColor;
					*((*decData) + writeOffset + 3) = 255;

					//ビットオフセットを更新
					bitOfs -= this->bitDepth_;
					if (bitOfs < 0) {
						//次の読み込み位置に更新
						bitOfs = 8 - this->bitDepth_;
						readOffset++;
					}
					//書き込み位置を更新
					writeOffset += BYTE_PER_PIXEL_RGBA8888;
				}
			}
		}
		else {
			//ビット深度が1bit,2bit,4bit,8bit以外の場合
			//ビット深度が16bitの可能性があるが、現状48bitカラーは表現できないので実装しない
			//16bit以外でここに来た場合は異常
			rc = -1;
		}

		return rc;
	}

	//トゥルーカラーPNG画像からRGBA8888画像へデコード
	std::int32_t DWImagePNG::decodeTrueColorPng_RGBA8888(std::uint8_t** const decData, const png_bytepp png, const bool isAlpha)
	{
		std::int32_t rc = 0;

		if (this->bitDepth_ == 8) {
			//ビット深度が8bitの場合

			//出力データへデコード後の画像データを設定
			for (std::int32_t h = 0; h < this->height_; h++) {
				//一行ずつ処理
				png_bytep wp = png[h];

				//書き込み位置と読み込み位置を初期化
				std::int32_t writeOffset = h * this->width_ * BYTE_PER_PIXEL_RGBA8888;
				std::int32_t readOffset = 0;

				for (std::int32_t w = 0; w < this->width_; w++) {
					//出力データへRGBA値を設定
					*((*decData) + writeOffset + 0) = wp[readOffset + 0];
					*((*decData) + writeOffset + 1) = wp[readOffset + 1];
					*((*decData) + writeOffset + 2) = wp[readOffset + 2];
					*((*decData) + writeOffset + 3) = (isAlpha) ? wp[readOffset + 3] : 255;

					//書き込み位置を更新
					writeOffset += BYTE_PER_PIXEL_RGBA8888;
					readOffset += (isAlpha) ? 4 : 3;
				}
			}
		}
		else {
			//ビット深度が8bit以外は何もしない
			//ビット深度が16bitの可能性があるが、現状48bitカラーは表現できないので実装しない
			//16bit以外でここに来た場合は異常
			rc = -1;
		}

		return rc;
	}

	//パレットPNG画像からRGBA8888画像へデコード
	std::int32_t DWImagePNG::decodePalletePng_RGBA8888(std::uint8_t** const decData, const png_bytepp png)
	{
		std::int32_t rc = 0;

		//PLTEチャンク読み込み
		png_colorp pallete = nullptr;
		std::int32_t palleteNum = 0;
		png_get_PLTE(this->pngStr_, this->pngInfo_, &pallete, &palleteNum);

		if ((pallete != nullptr) && (palleteNum > 0)) {
			//パレットデータ取得成功

			//出力データへデコード後の画像データを設定
			for (std::int32_t h = 0; h < this->height_; h++) {
				//一行ずつ処理
				png_bytep wp = png[h];

				//書き込み位置と読み込み位置を初期化
				std::int32_t writeOffset = h * this->width_ * BYTE_PER_PIXEL_RGBA8888;
				std::int32_t readOffset = 0;

				//ビット深度に応じたビットオフセットとビットマスク
				png_int_16 bitOfs = 8 - this->bitDepth_;
				png_byte bitMask = (0x01 << this->bitDepth_) - 1;

				for (std::int32_t w = 0; w < this->width_; w++) {
					//画像データはパレットインデックス
					png_byte palleteIndex = (wp[readOffset] >> bitOfs) & bitMask;

					//出力データへRGBA値を設定
					*((*decData) + writeOffset + 0) = pallete[palleteIndex].red;
					*((*decData) + writeOffset + 1) = pallete[palleteIndex].green;
					*((*decData) + writeOffset + 2) = pallete[palleteIndex].blue;
					*((*decData) + writeOffset + 3) = 255;

					//ビットオフセットを更新
					bitOfs -= this->bitDepth_;
					if (bitOfs < 0) {
						//次の読み込み位置に更新
						bitOfs = 8 - this->bitDepth_;
						readOffset++;
					}
					//書き込み位置を更新
					writeOffset += BYTE_PER_PIXEL_RGBA8888;
				}
			}
		}
		else {
			//パレットデータ取得失敗
			rc = -1;
		}

		return rc;
	}




	//----------------------------------------------------------------
	// DWFuncクラス
	//----------------------------------------------------------------

	//時刻取得
	DWTime DWFunc::getTime()
	{
		time_t t = time(nullptr);
		struct tm tm;
		(void)localtime_s(&tm, &t);

		DWTime dwTime = { 0 };
		dwTime.h_ = tm.tm_hour;
		dwTime.m_ = tm.tm_min;
		dwTime.s_ = tm.tm_sec;
		strftime(dwTime.str_, sizeof(dwTime.str_), "%H:%M:%S", &tm);
		dwTime.strNum_ = static_cast<std::int32_t>(strlen(dwTime.str_));

		return dwTime;
	}

	//数字画像ファイルパス取得
	std::string DWFunc::getFilePath_TimeNumImage(const std::char8_t timeNumber, DWImageFormat& format)
	{
		std::string filePath = "./image/";
		switch (timeNumber) {
		case '0':	filePath = filePath + "0_zero.png";		format = DWImageFormat::PNG;	break;
		case '1':	filePath = filePath + "1_one.png";		format = DWImageFormat::PNG;	break;
		case '2':	filePath = filePath + "2_two.png";		format = DWImageFormat::PNG;	break;
		case '3':	filePath = filePath + "3_three.png";	format = DWImageFormat::PNG;	break;
		case '4':	filePath = filePath + "4_four.png";		format = DWImageFormat::PNG;	break;
		case '5':	filePath = filePath + "5_five.png";		format = DWImageFormat::PNG;	break;
		case '6':	filePath = filePath + "6_six.png";		format = DWImageFormat::PNG;	break;
		case '7':	filePath = filePath + "7_seven.png";	format = DWImageFormat::PNG;	break;
		case '8':	filePath = filePath + "8_eight.png";	format = DWImageFormat::PNG;	break;
		case '9':	filePath = filePath + "9_nine.png";		format = DWImageFormat::PNG;	break;
		case ':':	filePath = filePath + "sym_colon.png";	format = DWImageFormat::PNG;	break;
		//case ':':	filePath = filePath + "win-24.bmp";		format = DWImageFormat::BMP;	break;
		default:	break;
		}
		return filePath;
	}
}
