#ifndef INCLUDED_DWMAIN_HPP
#define INCLUDED_DWMAIN_HPP

#include "DWType.hpp"
#include "DWUtility.hpp"
#include <thread>
#include <mutex>

namespace dw {

	//DWMainクラス
	class DWMain {
		enum TaskState {
			START,
			END,
		};

		//メンバ変数
		std::thread		th_;
		std::mutex		mtx_;
		TaskState		taskState_;

	public:
		//開始
		static void start();
		//終了
		static void terminate();

	private:
		//コンストラクタ
		DWMain();
		//デストラクタ
		~DWMain();
		//メインタスク
		void task();
	};
};

#endif //INCLUDED_DWMAIN_HPP
