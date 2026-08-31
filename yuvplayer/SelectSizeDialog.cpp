#include "stdafx.h"
#include "yuvplayer.h"
#include "SelectSizeDialog.h"

IMPLEMENT_DYNAMIC(CSelectSizeDialog, CDialog)

CSelectSizeDialog::CSelectSizeDialog(CWnd* pParent /*=NULL*/)
    : CDialog(CSelectSizeDialog::IDD, pParent)
    , m_selectedIdx(0)
    , m_isCustom(FALSE)
    , m_customW(352)
    , m_customH(288)
    , m_sizeWHcand(NULL)
    , m_nCand(0)
    , m_fileSize(0)
{
}

CSelectSizeDialog::~CSelectSizeDialog()
{
    for (int i = 0; i < m_radios.GetSize(); ++i)
        delete m_radios[i];
    m_radios.RemoveAll();
}

void CSelectSizeDialog::SetCandidates(const int (*sizeWHcand)[2], const int* candIdx, const __int64* candFrames, int nCand, __int64 fileSize)
{
    m_sizeWHcand = sizeWHcand;
    m_nCand = nCand;
    m_fileSize = fileSize;
    for (int i = 0; i < nCand && i < 64; ++i) {
        m_candIdx[i] = candIdx[i];
        m_candFrames[i] = candFrames[i];
    }
}

void CSelectSizeDialog::DoDataExchange(CDataExchange* pDX)
{
    CDialog::DoDataExchange(pDX);
    DDX_Text(pDX, IDC_EDIT_CUSTOM_W, m_customW);
    DDV_MinMaxUInt(pDX, m_customW, 1, 8192);
    DDX_Text(pDX, IDC_EDIT_CUSTOM_H, m_customH);
    DDV_MinMaxUInt(pDX, m_customH, 1, 8192);
}

BEGIN_MESSAGE_MAP(CSelectSizeDialog, CDialog)
    ON_CONTROL_RANGE(BN_CLICKED, 2000, 2064, OnRadioClicked)
    ON_EN_CHANGE(IDC_EDIT_CUSTOM_W, OnEnChangeCustom)
    ON_EN_CHANGE(IDC_EDIT_CUSTOM_H, OnEnChangeCustom)
END_MESSAGE_MAP()

BOOL CSelectSizeDialog::OnInitDialog()
{
    CDialog::OnInitDialog();

    CreateRadios();
    LayoutControls();
    UpdateCustomEnable();

    // default selection: first candidate
    if (m_radios.GetSize() > 0) {
        CButton* btn = (CButton*)GetDlgItem(2000);
        if (btn) btn->SetCheck(BST_CHECKED);
        m_selectedIdx = 0;
        m_isCustom = FALSE;
    }

    return TRUE;
}

void CSelectSizeDialog::CreateRadios()
{
    // create radio buttons dynamically inside dialog client area
    // positions in dialog units converted to pixels via MapDialogRect
    const int IDC_RADIO_BASE = 2000;
    const int startX = 7;
    const int startY = 20;
    const int dy = 12; // dialog units vertical spacing

    CFont* dlgFont = GetFont();
    for (int i = 0; i < m_nCand; ++i) {
        int w = m_sizeWHcand[m_candIdx[i]][0];
        int h = m_sizeWHcand[m_candIdx[i]][1];
        wchar_t label[64];
        swprintf(label, 64, L"%dx%d - %lld frames", w, h, m_candFrames[i]);

        CButton* btn = new CButton();
        CRect rc(startX, startY + i*dy, startX + 246, startY + i*dy + 10);
        MapDialogRect(&rc);
        DWORD style = WS_CHILD | WS_VISIBLE | BS_AUTORADIOBUTTON | WS_GROUP;
        if (i == 0) style |= WS_GROUP;
        else style &= ~WS_GROUP;
        // use BS_AUTORADIOBUTTON without WS_GROUP for subsequent to make them grouped
        if (i != 0) style = WS_CHILD | WS_VISIBLE | BS_AUTORADIOBUTTON;
        btn->Create(label, style, rc, this, IDC_RADIO_BASE + i);
        if (dlgFont) btn->SetFont(dlgFont, FALSE);
        m_radios.Add(btn);
    }

    // custom radio at bottom
    {
        int idx = m_nCand;
        CButton* btn = new CButton();
        CRect rc(startX, startY + idx*dy, startX + 45, startY + idx*dy + 10);
        MapDialogRect(&rc);
        DWORD style = WS_CHILD | WS_VISIBLE | BS_AUTORADIOBUTTON;
        btn->Create(L"Custom", style, rc, this, IDC_RADIO_BASE + idx);
        if (dlgFont) btn->SetFont(dlgFont, FALSE);
        m_radios.Add(btn);
    }
}

void CSelectSizeDialog::LayoutControls()
{
    // Move custom W/H edits and labels next to custom radio, and OK/Cancel below
    const int startY = 20;
    const int dy = 12;
    int customIdx = m_nCand;
    int customYdu = startY + customIdx*dy; // dialog units Y of custom radio

    // Convert dialog units to pixels for positioning
    CRect rcRadio(0,0,0,0);
    // Get custom radio rect to align edits vertically centered
    CButton* customBtn = (CButton*)GetDlgItem(2000 + customIdx);
    if (!customBtn) return;
    CRect rcCustom;
    customBtn->GetWindowRect(&rcCustom);
    ScreenToClient(&rcCustom);

    // Edits: place to the right of custom radio (approx 35 du offset, avoid overlap with widened Custom radio)
    CRect rcWdu(58, customYdu, 98, customYdu+14);
    CRect rcHdu(112, customYdu, 152, customYdu+14);
    CRect rcXdu(101, customYdu+2, 109, customYdu+10);
    CRect rcWLabDu(48, customYdu+2, 58, customYdu+10);
    CRect rcHLabDu(154, customYdu+2, 164, customYdu+10);
    MapDialogRect(&rcWdu);
    MapDialogRect(&rcHdu);
    MapDialogRect(&rcXdu);
    MapDialogRect(&rcWLabDu);
    MapDialogRect(&rcHLabDu);

    // Align vertically to custom radio center
    int centerY = rcCustom.top + rcCustom.Height()/2;
    auto centerVert = [&](CWnd* w, CRect duRc){
        CRect prc = duRc;
        int h = prc.Height();
        prc.top = centerY - h/2;
        prc.bottom = prc.top + h;
        w->MoveWindow(prc);
    };

    CWnd* wW = GetDlgItem(IDC_EDIT_CUSTOM_W);
    CWnd* wH = GetDlgItem(IDC_EDIT_CUSTOM_H);
    CWnd* wX = GetDlgItem(IDC_STATIC_X);
    CWnd* wWLab = GetDlgItem(IDC_STATIC_W);
    CWnd* wHLab = GetDlgItem(IDC_STATIC_H);
    if (wW) centerVert(wW, rcWdu);
    if (wH) centerVert(wH, rcHdu);
    if (wX) {
        CRect prc = rcXdu;
        int h = prc.Height();
        prc.top = centerY - h/2;
        prc.bottom = prc.top + h;
        wX->MoveWindow(prc);
    }
    if (wWLab) {
        CRect prc = rcWLabDu;
        int h = prc.Height();
        prc.top = centerY - h/2;
        prc.bottom = prc.top + h;
        wWLab->MoveWindow(prc);
    }
    if (wHLab) {
        CRect prc = rcHLabDu;
        int h = prc.Height();
        prc.top = centerY - h/2;
        prc.bottom = prc.top + h;
        wHLab->MoveWindow(prc);
    }

    // Move OK/Cancel below edits
    CWnd* btnOk = GetDlgItem(IDOK);
    CWnd* btnCancel = GetDlgItem(IDCANCEL);
    if (btnOk && btnCancel) {
        CRect rcOk, rcCancel;
        btnOk->GetWindowRect(&rcOk); ScreenToClient(&rcOk);
        btnCancel->GetWindowRect(&rcCancel); ScreenToClient(&rcCancel);
        int btnTop = rcCustom.bottom + 10;
        int btnH = rcOk.Height();
        rcOk.top = btnTop; rcOk.bottom = btnTop + btnH;
        rcCancel.top = btnTop; rcCancel.bottom = btnTop + btnH;
        btnOk->MoveWindow(rcOk);
        btnCancel->MoveWindow(rcCancel);

        // resize dialog height to fit buttons
        CRect dlgRect;
        GetWindowRect(&dlgRect);
        CRect clientRect;
        GetClientRect(&clientRect);
        int chromeH = dlgRect.Height() - clientRect.Height();
        int newClientH = rcOk.bottom + 7;
        // convert client height to window height
        CRect newWin = dlgRect;
        newWin.bottom = newWin.top + chromeH + newClientH;
        // limit minimum height for small nCand, and ensure not too large
        // keep at least original, but expand if needed
        MoveWindow(newWin);
    }
}

void CSelectSizeDialog::UpdateCustomEnable()
{
    BOOL isCustom = FALSE;
    CButton* customBtn = (CButton*)GetDlgItem(2000 + m_nCand);
    if (customBtn && customBtn->GetCheck() == BST_CHECKED)
        isCustom = TRUE;

    CWnd* wW = GetDlgItem(IDC_EDIT_CUSTOM_W);
    CWnd* wH = GetDlgItem(IDC_EDIT_CUSTOM_H);
    if (wW) wW->EnableWindow(isCustom);
    if (wH) wH->EnableWindow(isCustom);

    // optional: set focus to first edit when custom selected
    if (isCustom && wW) {
        wW->SetFocus();
        // select all text
        ((CEdit*)wW)->SetSel(0, -1);
    }
}

void CSelectSizeDialog::OnRadioClicked(UINT nID)
{
    int idx = (int)nID - 2000;
    if (idx < 0 || idx > m_nCand) return;

    // update check states (BS_AUTORADIOBUTTON handles, but ensure)
    for (int i = 0; i < m_radios.GetSize(); ++i) {
        CButton* b = m_radios[i];
        b->SetCheck(i == idx ? BST_CHECKED : BST_UNCHECKED);
    }
    m_selectedIdx = idx;
    m_isCustom = (idx == m_nCand);
    UpdateCustomEnable();
}

void CSelectSizeDialog::OnEnChangeCustom()
{
    // if user types in custom edits, auto-select custom radio
    CButton* customBtn = (CButton*)GetDlgItem(2000 + m_nCand);
    if (!customBtn) return;
    if (customBtn->GetCheck() != BST_CHECKED) {
        // check custom radio
        for (int i = 0; i < m_radios.GetSize(); ++i) {
            CButton* b = m_radios[i];
            b->SetCheck(i == m_nCand ? BST_CHECKED : BST_UNCHECKED);
        }
        m_selectedIdx = m_nCand;
        m_isCustom = TRUE;
        UpdateCustomEnable();
    }
}

void CSelectSizeDialog::OnOK()
{
    // if custom not selected, just close
    if (m_selectedIdx != m_nCand) {
        CDialog::OnOK();
        return;
    }
    // custom selected -> validate edits
    if (!UpdateData(TRUE)) return;

    if (m_customW < 1 || m_customW > 8192 || m_customH < 1 || m_customH > 8192) {
        AfxMessageBox(L"Width/Height must be 1..8192");
        return;
    }
    m_isCustom = TRUE;
    CDialog::OnOK();
}
