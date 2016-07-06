
// #define			AUDIO_CODEC_PATH				TEXT("Wow6432Node\\CLSID\\{083863F1-70DE-11D0-BD40-00A0C911CE86}\\Instance")
// #define			NERO6_AUDIO_DECODER_GUID		TEXT("{138130AF-A79B-45D5-B4AA-87697457BA87}")

#include <dshow.h>
#include <cstdio>

class CShellExecute
{

public:
		BOOL						PlayFile(LPCTSTR pszFile);			
};

class CPin
{
	IPin*							m_pPin;
	std::vector<AM_MEDIA_TYPE>		m_vMediaTypes;
	PIN_DIRECTION					m_pdPin;

	void							EnumMediaTypes();

public:
	CPin(IPin * pPin);
	~CPin();

	operator IPin*					();
	IPin *							Object();

	PIN_DIRECTION					Dir();
	void							MediaTypesList(std::vector<AM_MEDIA_TYPE> & amtList);
};

class CBaseFilter
{
	REGFILTER			m_rf;
	CString				m_sFilterName;
	IBaseFilter*		m_pBaseFilter;
	std::vector<CPin*>	m_vPinList;
	BOOL				m_fDontRelease;

	HRESULT				InitPins();

public:
	CBaseFilter(REGFILTER rf);
	CBaseFilter(IBaseFilter * pBaseFilter);
	~CBaseFilter();

	operator IBaseFilter*();


	IBaseFilter*		Object();
	BOOL				Init();
	void				PinList(std::vector<CPin*> & pinList);
	CString				Name();
	CLSID				Clsid();
};

class CBaseFilterEnumerator
{
	IFilterMapper					* m_pEnumerator;
	std::vector<CBaseFilter*>		m_vFiltersList;

	void							EnumerateFilters();

public:
	CBaseFilterEnumerator();
	~CBaseFilterEnumerator();

	void							FilterList(std::vector<CBaseFilter*> & fltList);
};

class CDSPlayer
{
	BOOL					m_fIfaceWorks;
    IGraphBuilder			*m_pGraph;
    IMediaControl			*m_pControl;
	IMediaPosition			*m_pPosition;
	IBasicAudio				*m_pAudio;
	CBaseFilterEnumerator	m_fltEnum;


	double					m_dPlayLength;

	BOOL					LoadInterfaces();
	BOOL					FreeInterfaces();
	HRESULT					TryConnectFilters(IGraphBuilder * pGraph, IBaseFilter * pSource);
	HRESULT					ConnectFilters(std::vector<CBaseFilter*> & vFltList, IGraphBuilder * pGraph, IBaseFilter * pSource);
	HRESULT					ConnectPins(std::vector<CBaseFilter*> & vFltList, IGraphBuilder * pGraph, IBaseFilter * pSource, CPin * pPinSource);
	void					ReleaseGraph();
	BOOL					IsFilterAllowed(CBaseFilter * pFilter);

public:
	CDSPlayer();
	~CDSPlayer();
	BOOL					PlayFile(LPCTSTR pszFilepath, LONG lVolume = 0, LONG lBalance = 0);
	BOOL					StopPlay();
	double					CurrentPos();
	void					CurrentPos(double pos);
	double					GetLength();
	LONG					Balance();
	LONG					Volume();
	void					Balance(LONG lBalance);
	void					Volume(LONG lVolume);
	BYTE					State();			// 0 - inactive, 1 - stopped, 2 - playing, 3 - failure
};