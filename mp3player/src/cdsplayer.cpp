
#include "stdafx.h"

#include <comdef.h>
#include <locale>
#include <dmodshow.h>
#include <dmoreg.h>
#include <ShellAPI.h>

// For IID_IGraphBuilder, IID_IMediaControl, IID_IMediaEvent
#pragma comment(lib, "strmiids.lib") 
#pragma comment(lib, "Dmoguids.lib")

BOOL CShellExecute::PlayFile(LPCTSTR pszFile)
{
	if ( int(ShellExecute(GetDesktopWindow(), TEXT("open"), pszFile, NULL, NULL, SW_SHOW)) <= 32 )
		return FALSE;

	return TRUE;
}

CDSPlayer::CDSPlayer()
	:
	m_fIfaceWorks( FALSE ),
	m_pGraph( NULL ),
	m_pControl( NULL ),
	m_pAudio( NULL )
{
    // Create the filter graph manager and query for interfaces.
	HRESULT hr = ::CoCreateInstance(CLSID_FilterGraph, NULL, 
					CLSCTX_INPROC_SERVER, IID_IGraphBuilder, (void **)&m_pGraph);
	if (SUCCEEDED(hr))
	{
		_tprintf(TEXT("Created m_pGraph: %p\n"), m_pGraph);
		m_fIfaceWorks = TRUE;
	}
}

BOOL CDSPlayer::LoadInterfaces()
{
	BOOL fSuccess = TRUE;
	fSuccess &= SUCCEEDED( m_pGraph->QueryInterface(IID_IMediaControl, (void **)&m_pControl) ); // ) _tprintf(TEXT("IMediaControl failed\n"));
	fSuccess &= SUCCEEDED( m_pGraph->QueryInterface(IID_IMediaPosition, (void **)&m_pPosition) ); // ) _tprintf(TEXT("IMediaPosition failed\n"));
	fSuccess &= SUCCEEDED( m_pGraph->QueryInterface(IID_IBasicAudio, (void**)&m_pAudio) ); // ) _tprintf(TEXT("IBasicAudio failed\n"));
	return fSuccess;
}

BOOL CDSPlayer::FreeInterfaces()
{
	if ( m_pControl )
		m_pControl->Release();
	if ( m_pPosition )
		m_pPosition->Release();
	if ( m_pAudio )
		m_pAudio->Release();

	m_pControl = NULL;
	m_pPosition = NULL;
	m_pAudio = NULL;

	return TRUE;
}

CDSPlayer::~CDSPlayer()
{
	if ( m_fIfaceWorks )
	{
		StopPlay();
		_tprintf(TEXT("Trying to release m_pGraph: %p..."), m_pGraph);
		m_pGraph->Release();
		_tprintf(TEXT(" OK\n"));
	}
	m_fIfaceWorks = FALSE;
}

void CDSPlayer::ReleaseGraph()
{
	IEnumFilters * pEnumFilters = NULL;
	IBaseFilter * pFilter = NULL;
	
	std::vector<IBaseFilter*> ldFltrs;

	if ( SUCCEEDED( m_pGraph->EnumFilters( &pEnumFilters ) ) )
	{
		ULONG c = 0;
		while( S_OK == pEnumFilters->Next(1, &pFilter, &c) )
		{
			FILTER_INFO fi = { 0 };

			pFilter->QueryFilterInfo(&fi);
			_tprintf(TEXT("Loaded filter (%p): '%s'.\n"), pFilter, fi.achName);
			if ( fi.pGraph )
					fi.pGraph->Release();

			ldFltrs.push_back( pFilter );
		}

		pEnumFilters->Release();
	}

	for(std::vector<IBaseFilter*>::iterator v = ldFltrs.begin(); v < ldFltrs.end(); ++v)
	{
		_tprintf(TEXT("Releasing filter (%p)... "), (*v));
		HRESULT hr = m_pGraph->RemoveFilter( *v );
		if ( SUCCEEDED( hr ) )
			_tprintf(TEXT("OK\n"));
		else
		{
			_com_error err( hr );
			_tprintf(TEXT("failed (0x%08x) [%s]\n"), hr, err.ErrorMessage());
		}
		(*v)->Release();
	}
}

BOOL CDSPlayer::PlayFile(LPCTSTR pszFile, LONG lVolume, LONG lBalance)
{
	if ( m_fIfaceWorks )
	{
		HRESULT hr = 0;
		StopPlay();

		IBaseFilter * pSource = NULL;

		if ( FAILED(m_pGraph->AddSourceFilter(pszFile, pszFile, &pSource)) )
			return FALSE;

		if ( SUCCEEDED( TryConnectFilters( m_pGraph, pSource ) ) )
		{

			// pSource->Release();
			LoadInterfaces();
			m_pPosition->put_CurrentPosition( 0 );
			m_pPosition->get_Duration( &m_dPlayLength );

			Volume(lVolume);
			Balance(lBalance);
			return SUCCEEDED( m_pControl->Run() );
		}

		pSource->Release();
	}

	return FALSE;
}

BOOL CDSPlayer::StopPlay()
{
	if ( m_fIfaceWorks && m_pControl )
	{
		HRESULT hr = m_pControl->Stop();
		ReleaseGraph();
		FreeInterfaces();
		return SUCCEEDED( hr );
	}

	return FALSE;
}

LONG CDSPlayer::Volume()
{
	if ( m_fIfaceWorks )
	{
		LONG lVolume = NULL;
		if ( SUCCEEDED( m_pAudio->get_Volume( &lVolume ) ) )
			return lVolume;

	}
	return 0;
}

LONG CDSPlayer::Balance()
{
	if ( m_fIfaceWorks )
	{
		LONG lBalance = NULL;
		if ( SUCCEEDED( m_pAudio->get_Balance( &lBalance ) ) )
			return lBalance;

	}
	return 0;
}

void CDSPlayer::Volume(LONG lVolume)
{
	if ( !m_fIfaceWorks )
		return;

	m_pAudio->put_Volume( lVolume );
}

void CDSPlayer::Balance(LONG lBalance)
{
	if ( !m_fIfaceWorks )
		return;

	m_pAudio->put_Balance( lBalance );
}

void CDSPlayer::CurrentPos(double pos)
{
	if ( !m_fIfaceWorks )
		return;

	m_pPosition->put_CurrentPosition( pos );
}

double CDSPlayer::CurrentPos()
{
	if ( !m_fIfaceWorks ) 
		return -1;
	
	REFTIME rf = { 0 };


	if ( SUCCEEDED( m_pPosition->get_CurrentPosition( &rf ) ) )
	{
		//_tprintf(TEXT("Pos: %0.5f\n"), rf);

		return rf;
	}
	return -1;
}

double CDSPlayer::GetLength()
{
	if ( !m_fIfaceWorks ) 
		return -1;
	
	return m_dPlayLength;
}

HRESULT CDSPlayer::TryConnectFilters(IGraphBuilder * pGraph, IBaseFilter * pSource)
{
//	setlocale(LC_ALL,"russian_russia.866");
//	SetConsoleCP(866); SetConsoleOutputCP(866);

	std::vector<CBaseFilter*> filters;
	m_fltEnum.FilterList(filters);

	return ConnectFilters(filters, pGraph, pSource);
}

HRESULT CDSPlayer::ConnectPins(std::vector<CBaseFilter*> & vFltList, IGraphBuilder * pGraph, IBaseFilter * pSource, CPin * pSourcePin)
{
	std::vector<AM_MEDIA_TYPE> vAmtSource;

	pSourcePin->MediaTypesList(vAmtSource);

	for(std::vector<CBaseFilter*>::iterator v = vFltList.begin(); v < vFltList.end(); ++v)
	{
		CBaseFilter * pFlt = *v;

		if ( !IsFilterAllowed( pFlt ) )
			continue;

		if ( pFlt->Init() )
		{
			std::vector<CPin*> vPinList;
			pFlt->PinList(vPinList);

			for(std::vector<CPin*>::iterator vp = vPinList.begin(); vp < vPinList.end(); ++vp)
			{
				CPin * pin = (*vp);
				if ( pin->Dir() == PINDIR_OUTPUT )
				{
					continue;
				}

				std::vector<AM_MEDIA_TYPE> vamt;
				pin->MediaTypesList(vamt);
				for(std::vector<AM_MEDIA_TYPE>::iterator va = vamt.begin(); va < vamt.end(); ++va)
				{
					for(std::vector<AM_MEDIA_TYPE>::iterator vas = vAmtSource.begin(); vas < vAmtSource.end(); ++vas)
					{
						if ( vas->majortype == va->majortype )
						{
							// we need to add this filter to graph before connect its pins
							pGraph->AddFilter(pFlt->Object(), pFlt->Name());
							HRESULT hrc = pGraph->ConnectDirect(pSourcePin->Object(), pin->Object(), NULL);
							if ( SUCCEEDED( hrc ) )
							{
								_tprintf(TEXT("Found suitlable filter '%s'!\n"), pFlt->Name());
								return ConnectFilters(vFltList, pGraph, pFlt->Object());
							}
							pGraph->RemoveFilter(pFlt->Object());
						}
					}
				}
			}
		}
	}
	return E_NOINTERFACE;
}

HRESULT CDSPlayer::ConnectFilters(std::vector<CBaseFilter*> & vFltList, IGraphBuilder * pGraph, IBaseFilter * pSource)
{
	HRESULT hr = E_NOINTERFACE;

	CPin * pSourcePin = NULL;

	CBaseFilter fltSource(pSource);

	// enumerate pins to find required OUTPUT pin
	std::vector<CPin*> pl;
	fltSource.PinList( pl );
	for(std::vector<CPin*>::iterator vpl = pl.begin(); vpl < pl.end(); ++vpl)
	{
		CPin * pin = *vpl;

		if ( pin->Dir() == PINDIR_OUTPUT )
		{
			pSourcePin = pin;
			hr = ( SUCCEEDED( ConnectPins(vFltList, pGraph, pSource, pin) ) ? S_OK : hr );
		}
	}

	return ( pSourcePin ? hr : S_OK );
}

BOOL CDSPlayer::IsFilterAllowed(CBaseFilter * pFilter)
{
	CString s = pFilter->Name();
	if ( s[0] == _T('N') &&
		 s[1] == _T('e') &&
		 s[2] == _T('r') &&
		 s[3] == _T('o') &&
		 s[4] == _T(' ')
		 )
		return FALSE;		// skip ugly nero filters

	return TRUE;

}

BYTE CDSPlayer::State()
{
	if ( !m_pControl )
		return 0;

	FILTER_STATE fs;

	if ( SUCCEEDED( m_pControl->GetState( 1000, (OAFilterState*) &fs ) ) )
	{
		switch( fs )
		{
		case State_Stopped: return 1;
		case State_Running: return 2;
		case State_Paused: return 3;	// paused? Here?!
		default: return 3;
		}
	}
	else
		return 3;
}

// ----------------------

CPin::CPin(IPin * pPin)
	:
	m_pPin( pPin )
{
	pPin->QueryDirection( &m_pdPin );
	EnumMediaTypes();
}

void CPin::EnumMediaTypes()
{
	IEnumMediaTypes * pEnumAmt = NULL;

	if ( SUCCEEDED( m_pPin->EnumMediaTypes( &pEnumAmt ) ) )
	{
		AM_MEDIA_TYPE * pamt = NULL;
		ULONG c = 0;
		while( S_OK == pEnumAmt->Next(1, &pamt, &c) )
		{
			m_vMediaTypes.push_back( *pamt );
		}
		pEnumAmt->Release();
	}
}

CPin::operator IPin *()
{
	return m_pPin;
}

IPin * CPin::Object()
{
	return m_pPin;
}

void CPin::MediaTypesList(std::vector<AM_MEDIA_TYPE> & vList)
{
	vList = m_vMediaTypes;
}

PIN_DIRECTION CPin::Dir()
{
	return m_pdPin;
}

CPin::~CPin()
{
	m_pPin->Release();
	m_pPin = NULL;
	m_vMediaTypes.clear();
}

CBaseFilter::CBaseFilter(REGFILTER rf)
	:
	m_rf( rf ),
	m_sFilterName( rf.Name ),
	m_pBaseFilter( NULL ),
	m_fDontRelease( FALSE )
{

}

CBaseFilter::CBaseFilter(IBaseFilter * pBaseFilter)
	:
	m_sFilterName( TEXT("") ),
	m_pBaseFilter( pBaseFilter ),
	m_fDontRelease( TRUE )
{
	pBaseFilter->GetClassID(&m_rf.Clsid);
	InitPins();
}

CBaseFilter::~CBaseFilter()
{
	if ( m_pBaseFilter )
	{
		long c = m_vPinList.size();
		for(long i = 0; i < c; ++i)
			delete (*(m_vPinList.begin() + i));

		m_vPinList.clear();
		
		if ( !m_fDontRelease )
			m_pBaseFilter->Release();

		m_pBaseFilter = NULL;
	}
}

CBaseFilter::operator IBaseFilter*()
{
	return m_pBaseFilter;
}

IBaseFilter * CBaseFilter::Object()
{
	return m_pBaseFilter;
}

CString CBaseFilter::Name()
{
	return m_sFilterName;
}

CLSID CBaseFilter::Clsid()
{
	return m_rf.Clsid;
}

BOOL CBaseFilter::Init()
{
	if ( m_pBaseFilter )
		return TRUE;

	HRESULT hr = CoCreateInstance(m_rf.Clsid, NULL, CLSCTX_INPROC_SERVER, IID_IBaseFilter, (void**) &m_pBaseFilter);
	if ( E_NOINTERFACE == hr ) // need to create wrapper for DMO
	{
		hr = CoCreateInstance(CLSID_DMOWrapperFilter, NULL, CLSCTX_INPROC_SERVER, IID_IBaseFilter, (void**) &m_pBaseFilter);
		if ( SUCCEEDED( hr ) )
		{
			IDMOWrapperFilter * pDMOFilter = NULL;
			hr = m_pBaseFilter->QueryInterface(IID_IDMOWrapperFilter, (void**) &pDMOFilter);
			if ( SUCCEEDED( hr ) )
			{
				pDMOFilter->Init(m_rf.Clsid, DMOCATEGORY_AUDIO_DECODER);
				pDMOFilter->Release();
			}
		}
	}

	if ( SUCCEEDED( hr ) )
		hr = InitPins();

	return SUCCEEDED( hr );
}

void CBaseFilter::PinList(std::vector<CPin*> & vList)
{
	vList = m_vPinList;
}

HRESULT CBaseFilter::InitPins()
{
	if ( !m_pBaseFilter )
		return E_POINTER;

	IEnumPins * pEnumPins = NULL;

	HRESULT hr = m_pBaseFilter->EnumPins( &pEnumPins );
	if ( SUCCEEDED( hr ) )
	{
		IPin * pPin = NULL;
		ULONG c = 0;
		while(S_OK == pEnumPins->Next(1, &pPin, &c))
		{
			CPin * pcPin = new CPin(pPin);
			m_vPinList.push_back( pcPin );
		}
		pEnumPins->Release();
	}

	return hr;
}


CBaseFilterEnumerator::CBaseFilterEnumerator()
{
	EnumerateFilters();
}

CBaseFilterEnumerator::~CBaseFilterEnumerator()
{
	long c = m_vFiltersList.size();

	for(long i = 0; i < c; ++i)
		delete (*(m_vFiltersList.begin() + i));

	m_vFiltersList.clear();
}

void CBaseFilterEnumerator::EnumerateFilters()
{
	IEnumRegFilters * pFilters = NULL;

	if ( SUCCEEDED( CoCreateInstance(CLSID_FilterMapper, NULL, CLSCTX_INPROC,IID_IFilterMapper, (LPVOID*) &m_pEnumerator) ) )
	{
		if ( SUCCEEDED( m_pEnumerator->EnumMatchingFilters( &pFilters, 0, FALSE, GUID_NULL, GUID_NULL, FALSE, FALSE, GUID_NULL, GUID_NULL ) ) )
		{
			REGFILTER * rf = NULL;
			ULONG c = 0;

			while( S_OK == pFilters->Next(1, &rf, &c) )
			{
				CBaseFilter * pFilter = new CBaseFilter( *rf );
				m_vFiltersList.push_back( pFilter );
			}
		}
	}
}

void CBaseFilterEnumerator::FilterList(std::vector<CBaseFilter*> & vList)
{
	vList = m_vFiltersList;
}
