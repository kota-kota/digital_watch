#ifndef INCLUDED_DWUTILITY_HPP
#define INCLUDED_DWUTILITY_HPP

#include "DWType.hpp"
#include <Windows.h>
#include <time.h>
#include <mutex>
#include <string>
#include <fstream>

//OpenGL
#include <gl/GL.h>

//FreeType
#include <ft2build.h>
#include FT_FREETYPE_H
#include <freetype/ftsynth.h>
#include <freetype/ftglyph.h>

//libpng
#include <png.h>
#include <pngstruct.h>

namespace dw {

	//DWWindowクラス
	class DWWindow {
		//フォント寸法情報
		struct FontMetrics {
			//メンバ変数
			std::int32_t	width_;		//フォント幅[pixel]
			std::int32_t	height_;	//フォント高さ[pixel]
			std::int32_t	offsetX_;	//グリフ原点(0,0)からグリフイメージの左端までの水平方向オフセット[pixel]
			std::int32_t	offsetY_;	//グリフ原点(0,0)からグリフイメージの上端までの垂直方向オフセット[pixel]
			std::int32_t	nextX_;		//次グリフへの水平方向オフセット[pixel]
			std::int32_t	nextY_;		//次グリフへの垂直方向オフセット[pixel]
			std::int32_t	kerningX_;	//水平方向カーニング
			std::int32_t	kerningY_;	//垂直方向カーニング
		};
		//フォントグリフ
		struct FontGlyph {
			FT_UInt			index_;		//グリフインデックス
			FT_Glyph		image_;		//グリフイメージ
			FontMetrics		metrics_;	//寸法情報
		};

		//メンバ変数
		HWND		hWnd_;
		HDC			hDC_;
		HGLRC		hGLRC_;

		FT_Library	ftLibrary_;
		FT_Face		ftFace_;

		DWSize		size_;

	public:
		//作成
		static void create(void* native);
		//取得
		static DWWindow* get();
		//破棄
		static void destroy();

		//描画開始
		void beginDraw();
		//描画終了
		void endDraw();
		//画面塗りつぶし
		void clear(const DWColor& color);
		//文字描画
		void drawText(const DWText& text, const DWCoord& coord, const DWColor& color);
		//画像描画
		void drawBitmap(const DWBitmap& bitmap, const DWCoord& coord);

	private:
		//コンストラクタ
		DWWindow(HWND hWnd);
		//デストラクタ
		~DWWindow();
	};

	//DWImageDecorderクラス
	class DWImageDecorder {
		//メンバ変数
		std::uint8_t*	decData_;		//デコードデータ
		std::int32_t	decDataSize_;	//デコードデータサイズ
		std::int32_t	width_;			//幅
		std::int32_t	height_;		//高さ

	public:
		//コンストラクタ
		DWImageDecorder();
		//デストラクタ
		~DWImageDecorder();
		//RGBA8888画像へデコード
		std::int32_t decode_RGBA8888(const std::char8_t* const bodyFilePath, const std::char8_t* const blendFilePath, const DWImageFormat format, const bool isFlip = false);
		std::int32_t decode_RGBA8888(std::uint8_t* const bodyData, const std::int32_t bodyDataSize, std::uint8_t* const blendData, const std::int32_t blendDataSize, const DWImageFormat format, const bool isFlip = false);
		//デコードデータ取得
		std::uint8_t* getDecodeData(std::int32_t* const decDataSize, std::int32_t* const width, std::int32_t* const height);

	private:
		//本体BMP画像をRGBA8888画像へデコード
		std::int32_t decodeBMP_RGBA8888(std::uint8_t* const bodyData, const std::int32_t bodyDataSize);
		//ブレンドBMP画像をRGBA8888画像へデコードし、本体デコード画像へブレンド
		std::int32_t blendBMP_RGBA8888(std::uint8_t* const blendData, const std::int32_t blendDataSize);
		//本体PNG画像をRGBA8888画像へデコード
		std::int32_t decodePNG_RGBA8888(std::uint8_t* const bodyData, const std::int32_t bodyDataSize);
		//ブレンドPNG画像をRGBA8888画像へデコードし、本体デコード画像へブレンド
		std::int32_t blendPNG_RGBA8888(std::uint8_t* const blendData, const std::int32_t blendDataSize);
		//RGBA8888画像のブレンド処理
		void blend_RGBA8888(std::uint8_t* const decData_blend);

		//コピーコンストラクタ(禁止)
		DWImageDecorder(const DWImageDecorder& org) = delete;
		//代入演算子(禁止)
		DWImageDecorder& operator=(const DWImageDecorder& org) = delete;
	};

	//DWImageBMPBMPクラス
	class DWImageBMP {

		//パレット最大数(256色)
		static const std::int32_t PALLETE_MAXNUM = 256;
		//Bitmapファイルヘッダ(Windows,OS/2共通)
		static const std::int32_t BFH_HEADERSIZE = 14;
		static const std::int32_t BFH_FILETYPE_OFS = 0;		//ファイルタイプ
		static const std::int32_t BFH_FILESIZE_OFS = 2;		//ファイルサイズ[byte]
		static const std::int32_t BFH_RESERVED1_OFS = 6;	//予約領域1
		static const std::int32_t BFH_RESERVED2_OFS = 8;	//予約領域2
		static const std::int32_t BFH_IMAGEOFS_OFS = 10;	//ファイル先頭から画像データまでのオフセット[byte]
															//Bitmap情報ヘッダ(Windows)
		static const std::int32_t BIH_HEADERSIZE = 40;
		static const std::int32_t BIH_HEADERSIZE_OFS = 14;	//情報ヘッダサイズ[byte]
		static const std::int32_t BIH_WIDTH_OFS = 18;		//画像の幅[pixel]
		static const std::int32_t BIH_HEIGHT_OFS = 22;		//画像の高さ[pixel]
		static const std::int32_t BIH_PLANENUM_OFS = 26;	//プレーン数
		static const std::int32_t BIH_BITCOUNT_OFS = 28;	//色ビット数[bit]
		static const std::int32_t BIH_COMPRESSION_OFS = 30;	//圧縮形式
		static const std::int32_t BIH_IMGDATASIZE_OFS = 34;	//画像データサイズ[byte]
		static const std::int32_t BIH_XDPM_OFS = 38;		//水平解像度[dot/m]
		static const std::int32_t BIH_YDPM_OFS = 42;		//垂直解像度[dot/m]
		static const std::int32_t BIH_PALLETENUM_OFS = 46;	//パレット数[使用色数]
		static const std::int32_t BIH_IMPCOLORNUM_OFS = 50;	//重要色数
		static const std::int32_t BIH_PALLETE_OFS = 54;		//パレット
															//Bitmapコアヘッダ(OS/2)
		static const std::int32_t BCH_HEADERSIZE = 12;
		static const std::int32_t BCH_HEADERSIZE_OFS = 14;	//情報ヘッダサイズ[byte]
		static const std::int32_t BCH_WIDTH_OFS = 18;		//画像の幅[pixel]
		static const std::int32_t BCH_HEIGHT_OFS = 20;		//画像の高さ[pixel]
		static const std::int32_t BCH_PLANENUM_OFS = 22;	//プレーン数
		static const std::int32_t BCH_BITCOUNT_OFS = 24;	//色ビット数[bit]
		static const std::int32_t BCH_PALLETE_OFS = 26;		//パレット
															//Bitmap圧縮形式
		static const std::uint32_t COMPRESSION_BI_RGB = 0;			//無圧縮
		static const std::uint32_t COMPRESSION_BI_RLE8 = 1;			//ランレングス圧縮[8bpp]
		static const std::uint32_t COMPRESSION_BI_RLE4 = 2;			//ランレングス圧縮[4bpp]
		static const std::uint32_t COMPRESSION_BI_BITFIELDS = 3;	//ビットフィールド

																	//パレットカラー構造体
		struct PalColor {
			std::uint8_t	r_;
			std::uint8_t	g_;
			std::uint8_t	b_;
		};

		//BMPフォーマット
		enum class BitmapFormat{
			INVALID,	//無効
			WIN,		//Windowsフォーマット
			OS2,		//OS/2フォーマット
		};

		//メンバ変数
		std::uint8_t*	bmp_;			//BMPデータ
		std::int32_t	bmpSize_;		//BMPデータサイズ
		BitmapFormat	format_;		//BMPタイプ
		std::int32_t	fileSize_;		//ファイルサイズ
		std::int32_t	imageOffset_;	//画像データまでのオフセット
		std::int32_t	width_;			//幅
		std::int32_t	height_;		//高さ
		std::int32_t	bitCount_;		//ビット数
		std::int32_t	compression_;	//圧縮形式
		std::int32_t	imageSize_;		//画像データサイズ
		std::int32_t	palleteNum_;	//パレット数
		std::int32_t	palleteByte_;	//1パレットあたりのバイト数
		std::int32_t	palleteOffset_;	//パレットまでのオフセット

	public:
		//コンストラクタ
		DWImageBMP();
		//デストラクタ
		~DWImageBMP();
		//作成
		std::int32_t create(std::uint8_t* const bmp, const std::int32_t bmpSize);
		//幅高さ取得
		void getWH(std::int32_t* const width, std::int32_t* const height);
		//RGBA8888画像へデコード
		std::int32_t decode_RGBA8888(std::uint8_t** const decData);

	private:
		//Bitmap情報ヘッダ(Windows)読み込み
		std::int32_t readInfoHeader_WINDOWS();
		//Bitmap情報ヘッダ(OS/2)読み込み
		std::int32_t readInfoHeader_OS2();
		//パディングバイト数を取得
		std::uint32_t getPaddingByte();
		//パレットデータを取得
		std::int32_t getPalleteData(PalColor* const pallete, const std::int32_t numMaxPal);
		//パレットBMP画像からRGBA8888画像へデコード
		std::int32_t decodePalleteBitmap_RGBA8888(std::uint8_t** const decData);
		//トゥルーカラーBitmap画像からRGBA8888画像へデコード
		std::int32_t decodeTrueColorBitmap_RGBA8888(std::uint8_t** const decData);

		//コピーコンストラクタ(禁止)
		DWImageBMP(const DWImageBMP& org) = delete;
		//代入演算子(禁止)
		DWImageBMP& operator=(const DWImageBMP& org) = delete;
	};

	//DWImagePNGクラス
	class DWImagePNG {

		//PNGシグネチャのバイト数
		static const std::int32_t PNG_BYTES_TO_CHECK = 4;

		//メンバ変数
		std::uint8_t*	png_;			//PNGデータ
		std::int32_t	pngSize_;		//PNGデータサイズ
		std::int32_t	width_;			//幅
		std::int32_t	height_;		//高さ
		std::int32_t	rowByte_;		//行バイト数
		std::uint8_t	bitDepth_;		//ビット深度
		std::uint8_t	colorType_;		//カラータイプ
		std::int16_t	dmy_;

		png_structp		pngStr_;		//PNG構造ポインタ(解放必要)
		png_infop		pngInfo_;		//PNG情報ポインタ(解放必要)

	public:
		//コンストラクタ
		DWImagePNG();
		//デストラクタ
		~DWImagePNG();
		//PNG読み込みコールバック関数
		static void callbackReadPng(png_structp pngStr, png_bytep data, png_size_t length);
		//作成
		std::int32_t create(std::uint8_t* const png, const std::int32_t pngSize);
		//幅高さ取得
		void getWH(std::int32_t* const width, std::int32_t* const height);
		//RGBA8888画像へデコード
		std::int32_t decode_RGBA8888(std::uint8_t** const decData);

	private:
		//グレーPNG画像からRGBA8888画像へデコード
		std::int32_t decodeGrayScalePng_RGBA8888(std::uint8_t** const decData, const png_bytepp png);
		//トゥルーカラーPNG画像からRGBA8888画像へデコード
		std::int32_t decodeTrueColorPng_RGBA8888(std::uint8_t** const decData, const png_bytepp png, const bool isAlpha);
		//パレットPNG画像からRGBA8888画像へデコード
		std::int32_t decodePalletePng_RGBA8888(std::uint8_t** const decData, const png_bytepp png);

		//コピーコンストラクタ(禁止)
		DWImagePNG(const DWImagePNG& org) = delete;
		//代入演算子(禁止)
		DWImagePNG& operator=(const DWImagePNG& org) = delete;
	};

	//DWFuncクラス
	class DWFunc {
	public:
		//時刻取得
		static DWTime getTime();
		//数字画像ファイルパス取得
		static std::string getFilePath_TimeNumImage(const std::char8_t timeNumber, DWImageFormat& format);
	};
};


#endif //INCLUDED_DWUTILITY_HPP
