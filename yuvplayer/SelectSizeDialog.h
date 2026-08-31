#pragma once

// CSelectSizeDialog dialog - shows YUV size candidates with inline custom w/h inputs

class CSelectSizeDialog : public CDialog
{
    DECLARE_DYNAMIC(CSelectSizeDialog)

public:
    CSelectSizeDialog(CWnd* pParent = NULL);
    virtual ~CSelectSizeDialog();

    enum { IDD = IDD_SELECT_SIZE };

    // inputs
    void SetCandidates(const int (*sizeWHcand)[2], const int* candIdx, const __int64* candFrames, int nCand, __int64 fileSize);
    void SetCustomInit(UINT w, UINT h) { m_customW = w; m_customH = h; }

    // outputs
    int  m_selectedIdx; // 0..nCand-1 for candidate, nCand for custom
    BOOL m_isCustom;
    UINT m_customW;
    UINT m_customH;

protected:
    virtual void DoDataExchange(CDataExchange* pDX);
    virtual BOOL OnInitDialog();
    virtual void OnOK();
    afx_msg void OnRadioClicked(UINT nID);
    afx_msg void OnEnChangeCustom();
    DECLARE_MESSAGE_MAP()

private:
    void UpdateCustomEnable();
    void CreateRadios();
    void LayoutControls();

    const int (*m_sizeWHcand)[2];
    int  m_candIdx[64];
    __int64 m_candFrames[64];
    int  m_nCand;
    __int64 m_fileSize;

    CFont m_radioFont;
    CArray<CButton*, CButton*> m_radios;
};
