
// MFCFFMpegSDLPlayDlg.h: 头文件
//

#pragma once
extern "C"
{
#include "libavcodec/avcodec.h"
#include "libavformat/avformat.h"
#include "libswscale/swscale.h"
#include "SDL2/SDL.h"
#include "libavutil\imgutils.h"
}
#include "FrameList.h"
// CMFCFFMpegSDLPlayDlg 对话框
class CMFCFFMpegSDLPlayDlg : public CDialogEx
{
// 构造
public:
	CMFCFFMpegSDLPlayDlg(CWnd* pParent = nullptr);	// 标准构造函数
	~CMFCFFMpegSDLPlayDlg();
// 对话框数据
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_MFCFFMPEGSDLPLAY_DIALOG };
#endif

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV 支持


// 实现
protected:
	HICON m_hIcon;

	// 生成的消息映射函数
	virtual BOOL OnInitDialog();
	afx_msg void OnSysCommand(UINT nID, LPARAM lParam);
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	DECLARE_MESSAGE_MAP()
public:
	afx_msg void OnBnClickedAbout();
	CEdit m_url;
	SDL_Window * m_screen;
	CWinThread* m_hPlayThread;
	CWinThread* m_hDecodeThread;
	CWinThread* m_hControlThread;
	CFrameQueue* m_lpFrameQueue;
	int m_w;
	int m_h;
	int m_exit;
	int m_pause;
	int m_screenW;
	int m_screenH;
	BOOL m_isPlay;
	afx_msg void OnBnClickedFiledialog();
	afx_msg void OnBnClickedPlay();
	afx_msg void OnBnClickedPause();
	afx_msg void OnBnClickedstop();
};
