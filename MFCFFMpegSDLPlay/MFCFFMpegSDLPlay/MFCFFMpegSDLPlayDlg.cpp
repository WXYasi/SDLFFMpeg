
// MFCFFMpegSDLPlayDlg.cpp: 实现文件
//

#include "pch.h"
#include "framework.h"
#include "MFCFFMpegSDLPlay.h"
#include "MFCFFMpegSDLPlayDlg.h"
#include "afxdialogex.h"

#define  __STDC_CONSTANT_MACROS



#ifdef _DEBUG
#define new DEBUG_NEW
#endif


// 用于应用程序“关于”菜单项的 CAboutDlg 对话框

class CAboutDlg : public CDialogEx
{
public:
	CAboutDlg();

// 对话框数据
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_ABOUTBOX };
#endif

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 支持

// 实现
protected:
	DECLARE_MESSAGE_MAP()
};

CAboutDlg::CAboutDlg() : CDialogEx(IDD_ABOUTBOX)
{
}

void CAboutDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CAboutDlg, CDialogEx)
END_MESSAGE_MAP()


// CMFCFFMpegSDLPlayDlg 对话框



CMFCFFMpegSDLPlayDlg::CMFCFFMpegSDLPlayDlg(CWnd* pParent /*=nullptr*/)
	: CDialogEx(IDD_MFCFFMPEGSDLPLAY_DIALOG, pParent)
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
	m_lpFrameQueue = new CFrameQueue();
	m_hControlThread = NULL;
	m_hDecodeThread = NULL;
	m_hPlayThread = NULL;
	m_screen = NULL;
	m_w = 0;
	m_h = 0;
	m_pause = 0;
	m_exit = 0;
	m_isPlay = FALSE;

}

CMFCFFMpegSDLPlayDlg::~CMFCFFMpegSDLPlayDlg()
{
	if (m_lpFrameQueue != NULL)
	{
		delete m_lpFrameQueue;
		m_lpFrameQueue = NULL;
	}
	m_hControlThread = NULL;
	m_hDecodeThread = NULL;
	m_hPlayThread = NULL;
	m_screen = NULL;
}
void CMFCFFMpegSDLPlayDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_EDIT1, m_url);
}

BEGIN_MESSAGE_MAP(CMFCFFMpegSDLPlayDlg, CDialogEx)
	ON_WM_SYSCOMMAND()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_BN_CLICKED(IDC_ABOUT, &CMFCFFMpegSDLPlayDlg::OnBnClickedAbout)
	ON_BN_CLICKED(IDC_FILEDIALOG, &CMFCFFMpegSDLPlayDlg::OnBnClickedFiledialog)
	ON_BN_CLICKED(IDC_PLAY, &CMFCFFMpegSDLPlayDlg::OnBnClickedPlay)
	ON_BN_CLICKED(IDC_PAUSE, &CMFCFFMpegSDLPlayDlg::OnBnClickedPause)
	ON_BN_CLICKED(IDC_stop, &CMFCFFMpegSDLPlayDlg::OnBnClickedstop)
END_MESSAGE_MAP()

int g_threadExit;
int framecnt2 = 0;
//Refresh Event
#define REFRESH_EVENT   (SDL_USEREVENT + 1)
//Break
#define BREAK_EVENT  (SDL_USEREVENT + 2)

#undef main



BOOL getWH(int & w, int& h, CMFCFFMpegSDLPlayDlg * cMFCDlg)
{

	char pcFilepath[500] = { 0 };
	int length = GetWindowTextLengthA(cMFCDlg->m_url);
	GetWindowTextA(cMFCDlg->m_url, (LPSTR)pcFilepath, length+1);
	AVFormatContext* pFormatCtx;

	pFormatCtx = avformat_alloc_context();

	int ret = avformat_open_input(&pFormatCtx, pcFilepath, NULL, NULL);

	avformat_find_stream_info(pFormatCtx, NULL);

	int video = -1;
	for (int i = 0; i < pFormatCtx->nb_streams; i++)
	{
		if (pFormatCtx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO)
		{
			video = i;
			break;
		}
	}

	AVCodecContext* pCodecCxt = avcodec_alloc_context3(NULL);
	avcodec_parameters_to_context(pCodecCxt, pFormatCtx->streams[video]->codecpar);

	h = pCodecCxt->height;
	w = pCodecCxt->width;

	avcodec_close(pCodecCxt);
	avformat_close_input(&pFormatCtx);

	return TRUE;
}
UINT UseSDLAndFFMpegPlay(LPVOID);

UINT refresh_video(LPVOID  lpParam) 
{
	CMFCFFMpegSDLPlayDlg* MFV_Param = (CMFCFFMpegSDLPlayDlg*)lpParam;

	getWH(MFV_Param->m_w, MFV_Param->m_h, (CMFCFFMpegSDLPlayDlg *)lpParam);

	MFV_Param->m_hControlThread = (CWinThread*)AfxBeginThread(UseSDLAndFFMpegPlay, (LPVOID)MFV_Param);

	SDL_Renderer* psdlRenderer = SDL_CreateRenderer(MFV_Param->m_screen, -1, 0);
	SDL_Texture* psdlTexture = SDL_CreateTexture(psdlRenderer, SDL_PIXELFORMAT_IYUV, SDL_TEXTUREACCESS_STREAMING, MFV_Param->m_w, MFV_Param->m_h);
	SDL_Rect sdlRect;
	sdlRect.x = 0;
	sdlRect.y = 0;
	sdlRect.w = MFV_Param->m_w;
	sdlRect.h = MFV_Param->m_h;
	BOOL rets = FALSE;
	int framecnt = 0;
	MSG msg;
	while (GetMessage(&msg, NULL, 0, 0)/*PeekMessage(&msg,NULL,0,0, PM_REMOVE)*/)//GetMessage在没有消息时是阻塞的，也就是不返回，线程被操作系统挂起。
	{
		//SDL_WaitEvent(&event);
		if (msg.message == WM_SIZE)
		{
			SDL_GetWindowSize(MFV_Param->m_screen, &MFV_Param->m_w, &MFV_Param->m_h);
		}
		else if(msg.message == REFRESH_EVENT)
		{
			Node* pNode = NULL;
			rets = MFV_Param->m_lpFrameQueue->pop(pNode);
			if (rets)
			{
				AVFrame* pFrameYUV = (AVFrame*)pNode->data;
				SDL_UpdateTexture(psdlTexture, NULL, pFrameYUV->data[0], pFrameYUV->linesize[0]);
				sdlRect.x = 0;
				sdlRect.y = 0;
				sdlRect.w = MFV_Param->m_w;
				sdlRect.h = MFV_Param->m_h;
				SDL_RenderClear(psdlRenderer);
				SDL_RenderCopy(psdlRenderer, psdlTexture, NULL, &sdlRect);
				SDL_RenderPresent(psdlRenderer);
				av_frame_free(&pFrameYUV);
				framecnt++;
			}
		}
		else if (msg.message == SDL_WINDOWEVENT_CLOSE)
		{
			SDL_Quit();
			break;
		}
	}


	//////////////////////////////////SDL////////////////////////////////////////
	return 1;
}

UINT ControlPlay(LPVOID  lpParam)
{
	CMFCFFMpegSDLPlayDlg* CMFCDlg = (CMFCFFMpegSDLPlayDlg*)lpParam;
	SDL_Event sdlEvent;
	framecnt2 = 0;
	sdlEvent.type = REFRESH_EVENT;
	while (!CMFCDlg->m_exit)
	{
		if(!CMFCDlg->m_pause)
		//SDL_PushEvent(&sdlEvent);
		::PostThreadMessageA(CMFCDlg->m_hPlayThread->m_nThreadID, REFRESH_EVENT, NULL, NULL);
		SDL_Delay(40);
		printf("11111111111 framecnt2 = %d\n", framecnt2);
		framecnt2++;
	}
	g_threadExit = 0;
	sdlEvent.type = BREAK_EVENT;
	SDL_PushEvent(&sdlEvent);
	return 0;
}

UINT UseSDLAndFFMpegPlay(LPVOID lpParam)
{
	CMFCFFMpegSDLPlayDlg * MPC_Param = (CMFCFFMpegSDLPlayDlg*)lpParam;
	char pcFilepath[500] = {0};
	GetWindowTextA(MPC_Param->m_url, (LPSTR)pcFilepath,500);
	AVFormatContext* pFormatCtx;
	avformat_network_init();

	pFormatCtx = avformat_alloc_context();

	//打开输入视频文件
	int ret = avformat_open_input(&pFormatCtx, pcFilepath, NULL, NULL);

	//获取视频文件信息
	avformat_find_stream_info(pFormatCtx, NULL);

	int video = -1;
	for (int i = 0; i < pFormatCtx->nb_streams; i++)
	{
		if (pFormatCtx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO)
		{
			video = i;
			break;
		}
	}

	AVCodecContext* pCodecCxt = avcodec_alloc_context3(NULL);
	avcodec_parameters_to_context(pCodecCxt, pFormatCtx->streams[video]->codecpar);
	
	//查找解码器
	AVCodec* pCodec = avcodec_find_decoder(pCodecCxt->codec_id);
	if (pCodec != NULL)
	{
		//打开解码器
		if (avcodec_open2(pCodecCxt, pCodec, NULL) < 0)
		{
			printf("open codec failed!\n");
			return -1;
		}
	}

	//打印出素材相关信息
	printf("===================================\n");
	av_dump_format(pFormatCtx, 0, pcFilepath, 0);
	printf("===================================\n");

	SwsContext* img_convert_ctx;
	//分配并返回一个 SwsContext。 您需要它来使用 sws_scale() 执行缩放/转换操作。
	if(MPC_Param->m_screenW > pCodecCxt->width && MPC_Param->m_screenH > pCodecCxt->height)
		img_convert_ctx = sws_getContext(pCodecCxt->width, pCodecCxt->height, pCodecCxt->pix_fmt,
			pCodecCxt->width, pCodecCxt->height, AV_PIX_FMT_YUV420P, SWS_BICUBIC, NULL, NULL, NULL);
	else
		img_convert_ctx = sws_getContext(pCodecCxt->width, pCodecCxt->height, pCodecCxt->pix_fmt,
			MPC_Param->m_screenW, MPC_Param->m_screenH, AV_PIX_FMT_YUV420P, SWS_BICUBIC, NULL, NULL, NULL);

	AVPacket* pPacket = (AVPacket*)av_malloc(sizeof(AVPacket));
	AVFrame* pFrame = av_frame_alloc();

	int framecnt = 0;
	while (!MPC_Param->m_exit)
	{
		//从输入文件读取一帧压缩数据
		if (av_read_frame(pFormatCtx, pPacket) >= 0)
		{
			if (pPacket->stream_index == video)
			{
				//向解码器送一帧压缩数据
				ret = avcodec_send_packet(pCodecCxt, pPacket);
				if (ret < 0)
				{
					printf("Decode Error.\n");
					return -1;
				}
				//从解码器中读取一帧解码数据
				while (avcodec_receive_frame(pCodecCxt, pFrame) == 0)
				{
					AVFrame* pFrameYUV = av_frame_alloc();
					uint8_t* out_buffer = (uint8_t*)av_malloc(av_image_get_buffer_size(AV_PIX_FMT_YUV420P, pCodecCxt->width, pCodecCxt->height, 1));
					av_image_fill_arrays(pFrameYUV->data, pFrameYUV->linesize, out_buffer, AV_PIX_FMT_YUV420P, pCodecCxt->width, pCodecCxt->height, 1);
					sws_scale(img_convert_ctx, (const uint8_t* const*)pFrame->data, pFrame->linesize, 0, pCodecCxt->height,
						pFrameYUV->data, pFrameYUV->linesize);
					printf("Decoded frame index: %d\n", framecnt);

					Node* pNode = new Node();
					pNode->data = (void*)pFrameYUV;
					pNode->framecnt = framecnt;
					framecnt++;
					do
					{
						ret = MPC_Param->m_lpFrameQueue->push(pNode);
					} while (!ret && !MPC_Param->m_exit);
				}
			}
			av_packet_unref(pPacket);
		}
	}
	sws_freeContext(img_convert_ctx);
	av_frame_free(&pFrame);
	avcodec_close(pCodecCxt);
	avformat_close_input(&pFormatCtx);

	return 1;
}

// CMFCFFMpegSDLPlayDlg 消息处理程序

BOOL CMFCFFMpegSDLPlayDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	// 将“关于...”菜单项添加到系统菜单中。

	// IDM_ABOUTBOX 必须在系统命令范围内。
	ASSERT((IDM_ABOUTBOX & 0xFFF0) == IDM_ABOUTBOX);
	ASSERT(IDM_ABOUTBOX < 0xF000);

	CMenu* pSysMenu = GetSystemMenu(FALSE);
	if (pSysMenu != nullptr)
	{
		BOOL bNameValid;
		CString strAboutMenu;
		bNameValid = strAboutMenu.LoadString(IDS_ABOUTBOX);
		ASSERT(bNameValid);
		if (!strAboutMenu.IsEmpty())
		{
			pSysMenu->AppendMenu(MF_SEPARATOR);
			pSysMenu->AppendMenu(MF_STRING, IDM_ABOUTBOX, strAboutMenu);
		}
	}

	// 设置此对话框的图标。  当应用程序主窗口不是对话框时，框架将自动
	//  执行此操作
	SetIcon(m_hIcon, TRUE);			// 设置大图标
	SetIcon(m_hIcon, FALSE);		// 设置小图标

	// TODO: 在此添加额外的初始化代码

	return TRUE;  // 除非将焦点设置到控件，否则返回 TRUE
}

void CMFCFFMpegSDLPlayDlg::OnSysCommand(UINT nID, LPARAM lParam)
{
	if ((nID & 0xFFF0) == IDM_ABOUTBOX)
	{
		CAboutDlg dlgAbout;
		dlgAbout.DoModal();
	}
	else
	{
		CDialogEx::OnSysCommand(nID, lParam);
	}
}

// 如果向对话框添加最小化按钮，则需要下面的代码
//  来绘制该图标。  对于使用文档/视图模型的 MFC 应用程序，
//  这将由框架自动完成。

void CMFCFFMpegSDLPlayDlg::OnPaint()
{
	if (IsIconic())
	{
		CPaintDC dc(this); // 用于绘制的设备上下文

		SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);

		// 使图标在工作区矩形中居中
		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;

		// 绘制图标
		dc.DrawIcon(x, y, m_hIcon);
	}
	else
	{
		CDialogEx::OnPaint();
	}
}

//当用户拖动最小化窗口时系统调用此函数取得光标
//显示。
HCURSOR CMFCFFMpegSDLPlayDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}



void CMFCFFMpegSDLPlayDlg::OnBnClickedAbout()
{
	// TODO: 在此添加控件通知处理程序代码
	CAboutDlg dlg1;
	dlg1.DoModal();
}


void CMFCFFMpegSDLPlayDlg::OnBnClickedFiledialog()
{
	// TODO: 在此添加控件通知处理程序代码

	CString FilePathName;
	CFileDialog dlg(TRUE, NULL, NULL, NULL, NULL);
	if (dlg.DoModal() == IDOK) {
		FilePathName = dlg.GetPathName();
		m_url.SetWindowTextW(FilePathName);
	}
}


void CMFCFFMpegSDLPlayDlg::OnBnClickedPlay()
{
	SDL_Init(SDL_INIT_VIDEO);
	m_screen = SDL_CreateWindowFrom(this->GetDlgItem(IDC_SCREEN)->GetSafeHwnd());
	CRect rect;
	CWnd* pWnd = this->GetDlgItem(IDC_SCREEN);
	pWnd->GetClientRect(&rect);//rc为控件的大小。
	m_screenH = rect.Height();
	m_screenW = rect.Width();

	if (m_isPlay == FALSE)
	{
		m_exit = 0;
		m_pause = 0;
		m_hPlayThread = (CWinThread*)AfxBeginThread(refresh_video, this);
		m_hControlThread = (CWinThread*)AfxBeginThread(ControlPlay, this);
		m_isPlay = TRUE;
	}
	else
	{
		WaitForSingleObject(m_hPlayThread, INFINITE);
		WaitForSingleObject(m_hControlThread, INFINITE);
		WaitForSingleObject(m_hDecodeThread, INFINITE);
		if (m_lpFrameQueue)
		{
			m_lpFrameQueue->Clear();
		}
		m_exit = 0;
		m_pause = 0;
		m_hPlayThread = (CWinThread*)AfxBeginThread(refresh_video, this);
		m_hControlThread = (CWinThread*)AfxBeginThread(ControlPlay, this);
		m_isPlay = TRUE;
	}
}


void CMFCFFMpegSDLPlayDlg::OnBnClickedPause()
{
	m_pause = !m_pause;
	// TODO: 在此添加控件通知处理程序代码
}


void CMFCFFMpegSDLPlayDlg::OnBnClickedstop()
{
	m_exit = 1;
	// TODO: 在此添加控件通知处理程序代码
}
