/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*-
 *
 * The contents of this file are subject to the Netscape Public
 * License Version 1.1 (the "License"); you may not use this file
 * except in compliance with the License. You may obtain a copy of
 * the License at http://www.mozilla.org/NPL/
 *
 * Software distributed under the License is distributed on an "AS
 * IS" basis, WITHOUT WARRANTY OF ANY KIND, either express or
 * implied. See the License for the specific language governing
 * rights and limitations under the License.
 *
 * The Original Code is mozilla.org code.
 *
 * The Initial Developer of the Original Code is Netscape
 * Communications Corporation.  Portions created by Netscape are
 * Copyright (C) 1998 Netscape Communications Corporation. All
 * Rights Reserved.
 *
 * Contributor(s): 
 *   Chak Nanga <chak@netscape.com> 
 */

#include "stdafx.h"
#include "Dialogs.h"
#include "BrowserView.h"
#include "MfcEmbed.h"
extern CMfcEmbedApp theApp;

// File overview....
//
// Contains find dialog stuff
//

//--------------------------------------------------------------------------//
//				CFindDialog Stuff
//--------------------------------------------------------------------------//

CFindDialog::CFindDialog(CString& csSearchStr, PRBool bMatchCase,
				PRBool bMatchWholeWord, PRBool bWrapAround,
				PRBool bSearchBackwards, CBrowserView* pOwner)
				: CFindReplaceDialog()
{
	// Save these initial settings off in member vars
	// We'll use these to initialize the controls
	// in InitDialog()

	m_csSearchStr = csSearchStr;
	m_bMatchCase = bMatchCase;
	m_bMatchWholeWord = bMatchWholeWord;
	m_bWrapAround = bWrapAround;
	m_bSearchBackwards = bSearchBackwards;
	m_pOwner = pOwner;

	// Set up to load our customized Find dialog template
	// rather than the default one MFC provides
	m_fr.Flags |= FR_ENABLETEMPLATE;
	m_fr.hInstance = AfxGetInstanceHandle();
	m_fr.lpTemplateName = MAKEINTRESOURCE(IDD_FINDDLG);
}

BOOL CFindDialog::OnInitDialog() 
{
	CFindReplaceDialog::OnInitDialog();

	CEdit* pEdit = (CEdit *)GetDlgItem(IDC_FIND_EDIT);
	if(pEdit)
		pEdit->SetWindowText(m_csSearchStr);

	CButton* pChk = (CButton *)GetDlgItem(IDC_MATCH_CASE);
	if(pChk)
		pChk->SetCheck(m_bMatchCase);

	pChk = (CButton *)GetDlgItem(IDC_MATCH_WHOLE_WORD);
	if(pChk)
		pChk->SetCheck(m_bMatchWholeWord);

   pChk = (CButton *)GetDlgItem(IDC_WRAP_AROUND);	
	if(pChk)
		pChk->SetCheck(m_bWrapAround);

	pChk = (CButton *)GetDlgItem(IDC_SEARCH_BACKWARDS);
	if(pChk)
		pChk->SetCheck(m_bSearchBackwards);

	return TRUE; 
}

void CFindDialog::PostNcDestroy()	
{
	// Let the owner know we're gone
	if(m_pOwner != NULL)	
		m_pOwner->ClearFindDialog();

	CFindReplaceDialog::PostNcDestroy();
}

BOOL CFindDialog::WrapAround()
{
	CButton* pChk = (CButton *)GetDlgItem(IDC_WRAP_AROUND);

	return pChk ? pChk->GetCheck() : FALSE;
}

BOOL CFindDialog::SearchBackwards()
{
	CButton* pChk = (CButton *)GetDlgItem(IDC_SEARCH_BACKWARDS);

	return pChk ? pChk->GetCheck() : FALSE;
}

#include "Dialogs.h"

// File overview....
//
// Contains dialog box code to support Alerts, Prompts such as
// password prompt and username/password prompts
//

//--------------------------------------------------------------------------//
//				CPromptDialog Stuff
//--------------------------------------------------------------------------//

CPromptDialog::CPromptDialog(CWnd* pParent, const char* pTitle, const char* pText,
                             const char* pInitPromptText,
                             BOOL bHasCheck, const char* pCheckText, int initCheckVal)
    : CDialog(CPromptDialog::IDD, pParent),
    m_bHasCheckBox(bHasCheck)
{   
    if(pTitle)
        m_csDialogTitle = pTitle;
    if(pText)
        m_csPromptText = pText;
    if(pInitPromptText)
        m_csPromptAnswer = pInitPromptText;
    if(pCheckText)
        m_csCheckBoxText = pCheckText; 
}

void CPromptDialog::DoDataExchange(CDataExchange* pDX)
{
    CDialog::DoDataExchange(pDX);
    //{{AFX_DATA_MAP(CPromptDialog)
    DDX_Text(pDX, IDC_PROMPT_ANSWER, m_csPromptAnswer);
    DDX_Check(pDX, IDC_CHECK_SAVE_PASSWORD, m_bCheckBoxValue);
    //}}AFX_DATA_MAP
}

BEGIN_MESSAGE_MAP(CPromptDialog, CDialog)
    //{{AFX_MSG_MAP(CPromptDialog)
        // NOTE: the ClassWizard will add message map macros here
    //}}AFX_MSG_MAP
END_MESSAGE_MAP()

int CPromptDialog::OnInitDialog()
{   
    SetWindowText(m_csDialogTitle);
  
    CWnd *pWnd = GetDlgItem(IDC_PROMPT_TEXT);
    if(pWnd)
        pWnd->SetWindowText(m_csPromptText);

    CButton *pChk = (CButton *)GetDlgItem(IDC_CHECK_SAVE_PASSWORD);
    if(pChk)
    {
        if (m_bHasCheckBox)
        {
            if(!m_csCheckBoxText.IsEmpty())
                pChk->SetWindowText(m_csCheckBoxText);
            pChk->SetCheck(m_bCheckBoxValue ? BST_CHECKED : BST_UNCHECKED);
        }
        else
        {
            // Hide the check box control if there's no label text
            // This will be the case when we're not using single sign-on
            pChk->ShowWindow(SW_HIDE); 
        }
    }

    CEdit *pEdit = (CEdit *)GetDlgItem(IDC_PROMPT_ANSWER);
    if(pEdit) 
    {
        pEdit->SetWindowText(m_csPromptAnswer);
        pEdit->SetFocus();
        pEdit->SetSel(0, -1);

        return 0; // Returning "0" since we're explicitly setting focus
    }

    return TRUE;
}

//--------------------------------------------------------------------------//
//				CPromptPasswordDialog Stuff
//--------------------------------------------------------------------------//

CPromptPasswordDialog::CPromptPasswordDialog(CWnd* pParent, const char* pTitle, const char* pText,
                                             const char* pInitPasswordText,
                                             BOOL bHasCheck, const char* pCheckText, int initCheckVal)
    : CDialog(CPromptPasswordDialog::IDD, pParent),
    m_bHasCheckBox(bHasCheck), m_bCheckBoxValue(initCheckVal)
{   
	if(pTitle)
		m_csDialogTitle = pTitle;
	if(pText)
		m_csPromptText = pText;
	if(pInitPasswordText)
	    m_csPassword = pInitPasswordText;
	if(pCheckText)
		m_csCheckBoxText = pCheckText;
}

void CPromptPasswordDialog::DoDataExchange(CDataExchange* pDX)
{
    CDialog::DoDataExchange(pDX);
    //{{AFX_DATA_MAP(CPromptPasswordDialog)
    DDX_Text(pDX, IDC_PASSWORD, m_csPassword);
    DDX_Check(pDX, IDC_CHECK_SAVE_PASSWORD, m_bCheckBoxValue);
    //}}AFX_DATA_MAP
}

BEGIN_MESSAGE_MAP(CPromptPasswordDialog, CDialog)
    //{{AFX_MSG_MAP(CPromptPasswordDialog)
        // NOTE: the ClassWizard will add message map macros here
    //}}AFX_MSG_MAP
END_MESSAGE_MAP()

int CPromptPasswordDialog::OnInitDialog()
{   
    SetWindowText(m_csDialogTitle);
  
    CWnd *pWnd = GetDlgItem(IDC_PROMPT_TEXT);
    if(pWnd)
        pWnd->SetWindowText(m_csPromptText);

    CButton *pChk = (CButton *)GetDlgItem(IDC_CHECK_SAVE_PASSWORD);
    if(pChk)
    {
        if (m_bHasCheckBox)
        {
            if(!m_csCheckBoxText.IsEmpty())
                pChk->SetWindowText(m_csCheckBoxText);
            pChk->SetCheck(m_bCheckBoxValue ? BST_CHECKED : BST_UNCHECKED);
        }
        else
        {
            // Hide the check box control if there's no label text
            // This will be the case when we're not using single sign-on
            pChk->ShowWindow(SW_HIDE); 
        }
    }

    CEdit *pEdit = (CEdit *)GetDlgItem(IDC_PASSWORD);
    if(pEdit) 
    {
        pEdit->SetFocus();

        return 0; // Returning "0" since we're explicitly setting focus
    }

    return TRUE;
}

//--------------------------------------------------------------------------//
//				CPromptUsernamePasswordDialog Stuff
//--------------------------------------------------------------------------//

CPromptUsernamePasswordDialog::CPromptUsernamePasswordDialog(CWnd* pParent, const char* pTitle, const char* pText,
                                  const char* pInitUsername, const char* pInitPassword, 
		                          BOOL bHasCheck, const char* pCheckText, int initCheckVal)
    : CDialog(CPromptUsernamePasswordDialog::IDD, pParent),
    m_bHasCheckBox(bHasCheck), m_bCheckBoxValue(initCheckVal)
{
    if(pTitle)
        m_csDialogTitle = pTitle;
    if(pText)
        m_csPromptText = pText;
    if(pInitUsername)
        m_csUserName = pInitUsername;
    if(pInitPassword)
        m_csPassword = pInitPassword;
    if(pCheckText)
        m_csCheckBoxText = pCheckText;
}

void CPromptUsernamePasswordDialog::DoDataExchange(CDataExchange* pDX)
{
    CDialog::DoDataExchange(pDX);
    //{{AFX_DATA_MAP(CPromptUsernamePasswordDialog)
    DDX_Text(pDX, IDC_USERNAME, m_csUserName);
    DDX_Text(pDX, IDC_PASSWORD, m_csPassword);
    DDX_Check(pDX, IDC_CHECK_SAVE_PASSWORD, m_bCheckBoxValue);
    //}}AFX_DATA_MAP
}

BEGIN_MESSAGE_MAP(CPromptUsernamePasswordDialog, CDialog)
    //{{AFX_MSG_MAP(CPromptUsernamePasswordDialog)
        // NOTE: the ClassWizard will add message map macros here
    //}}AFX_MSG_MAP
END_MESSAGE_MAP()

int CPromptUsernamePasswordDialog::OnInitDialog()
{   
   	SetWindowText(m_csDialogTitle);
  
    CWnd *pWnd = GetDlgItem(IDC_PROMPT_TEXT);
    if(pWnd)
        pWnd->SetWindowText(m_csPromptText);

    CButton *pChk = (CButton *)GetDlgItem(IDC_CHECK_SAVE_PASSWORD);
    if(pChk)
    {
        if(m_bHasCheckBox)
        {
            if (!m_csCheckBoxText.IsEmpty())
                pChk->SetWindowText(m_csCheckBoxText);
            pChk->SetCheck(m_bCheckBoxValue ? BST_CHECKED : BST_UNCHECKED);
        }
        else
        {
            pChk->ShowWindow(SW_HIDE);
        }
    }

    CEdit *pEdit = (CEdit *)GetDlgItem(IDC_PASSWORD);
    if(pEdit) 
    {
        pEdit->SetWindowText(m_csPassword);
    }

    pEdit = (CEdit *)GetDlgItem(IDC_USERNAME);
    if(pEdit) 
    {
        pEdit->SetWindowText(m_csUserName);
        pEdit->SetSel(0, -1);

        pEdit->SetFocus();

        return 0; // Returning "0" since we're explicitly setting focus
    }

    return TRUE;
}

//--------------------------------------------------------------------------//
//				CAlertCheckDialog Stuff
//--------------------------------------------------------------------------//

CAlertCheckDialog::CAlertCheckDialog(CWnd* pParent, const char* pTitle, const char* pText,
                             const char* pCheckText, int initCheckVal)
        : CDialog(CAlertCheckDialog::IDD, pParent)
{   
    if(pTitle)
        m_csDialogTitle = pTitle;
    if(pText)
        m_csMsgText = pText;
    if(pCheckText)
        m_csCheckBoxText = pCheckText; 

    m_bCheckBoxValue = initCheckVal;
}

void CAlertCheckDialog::DoDataExchange(CDataExchange* pDX)
{
    CDialog::DoDataExchange(pDX);
    //{{AFX_DATA_MAP(CAlertCheckDialog)
    DDX_Check(pDX, IDC_CHECKBOX, m_bCheckBoxValue);
    //}}AFX_DATA_MAP
}

BEGIN_MESSAGE_MAP(CAlertCheckDialog, CDialog)
    //{{AFX_MSG_MAP(CAlertCheckDialog)
        // NOTE: the ClassWizard will add message map macros here
    //}}AFX_MSG_MAP
END_MESSAGE_MAP()

int CAlertCheckDialog::OnInitDialog()
{
    SetWindowText(m_csDialogTitle);

    CWnd *pWnd = GetDlgItem(IDC_MSG_TEXT);
    if(pWnd)
        pWnd->SetWindowText(m_csMsgText);

    CButton *pChk = (CButton *)GetDlgItem(IDC_CHECKBOX);
    if(pChk)
    {
        pChk->SetWindowText(m_csCheckBoxText);
        pChk->SetCheck(m_bCheckBoxValue ? BST_CHECKED : BST_UNCHECKED);
    }

    return TRUE;
}

//--------------------------------------------------------------------------//
//				CConfirmCheckDialog Stuff
//--------------------------------------------------------------------------//

CConfirmCheckDialog::CConfirmCheckDialog(CWnd* pParent, const char* pTitle, const char* pText,
                            const char* pCheckText, int initCheckVal,
                            const char *pBtn1Text, const char *pBtn2Text, 
                            const char *pBtn3Text)
            : CDialog(CConfirmCheckDialog::IDD, pParent)
{   
    if(pTitle)
        m_csDialogTitle = pTitle;
    if(pText)
        m_csMsgText = pText;
    if(pCheckText)
        m_csCheckBoxText = pCheckText; 

    m_bCheckBoxValue = initCheckVal;

    if(pBtn1Text)
        m_csBtn1Text = pBtn1Text;
    if(pBtn2Text)
        m_csBtn2Text = pBtn2Text;
    if(pBtn3Text)
        m_csBtn3Text = pBtn3Text;
}

void CConfirmCheckDialog::DoDataExchange(CDataExchange* pDX)
{
    CDialog::DoDataExchange(pDX);
    //{{AFX_DATA_MAP(CConfirmCheckDialog)
    DDX_Check(pDX, IDC_CHECKBOX, m_bCheckBoxValue);
    //}}AFX_DATA_MAP
}

BEGIN_MESSAGE_MAP(CConfirmCheckDialog, CDialog)
    //{{AFX_MSG_MAP(CConfirmCheckDialog)
    ON_BN_CLICKED(IDC_BTN1, OnBtn1Clicked)
    ON_BN_CLICKED(IDC_BTN2, OnBtn2Clicked)
    ON_BN_CLICKED(IDC_BTN3, OnBtn3Clicked)
    //}}AFX_MSG_MAP
END_MESSAGE_MAP()

int CConfirmCheckDialog::OnInitDialog()
{   
   	SetWindowText(m_csDialogTitle);
  
    CWnd *pWnd = GetDlgItem(IDC_MSG_TEXT);
    if(pWnd)
        pWnd->SetWindowText(m_csMsgText);

    CButton *pChk = (CButton *)GetDlgItem(IDC_CHECKBOX);
    if(pChk)
    {
        if(m_csCheckBoxText.IsEmpty())
        {
            pChk->ShowWindow(SW_HIDE);
        }
        else
        {
            pChk->SetWindowText(m_csCheckBoxText);
            pChk->SetCheck(m_bCheckBoxValue ? BST_CHECKED : BST_UNCHECKED);
        }
    }

    CButton *pBtn1 = (CButton *)GetDlgItem(IDC_BTN1);
    if(pBtn1)
    {
        if(m_csBtn1Text.IsEmpty())
            pBtn1->ShowWindow(SW_HIDE);
        else
            pBtn1->SetWindowText(m_csBtn1Text);
    }

    CButton *pBtn2 = (CButton *)GetDlgItem(IDC_BTN2);
    if(pBtn2)
    {
        if(m_csBtn2Text.IsEmpty())
            pBtn2->ShowWindow(SW_HIDE);
        else
            pBtn2->SetWindowText(m_csBtn2Text);
    }

    CButton *pBtn3 = (CButton *)GetDlgItem(IDC_BTN3);
    if(pBtn3)
    {
        if(m_csBtn3Text.IsEmpty())
            pBtn3->ShowWindow(SW_HIDE);
        else
            pBtn3->SetWindowText(m_csBtn3Text);
    }

    return TRUE;
}

void CConfirmCheckDialog::OnBtn1Clicked()
{
    UpdateData();

    EndDialog(0); // where 0 indicates that the btn pressed was at index 0
}

void CConfirmCheckDialog::OnBtn2Clicked()
{
    UpdateData();

    EndDialog(1); // where 1 indicates that the btn pressed was at index 1
}

void CConfirmCheckDialog::OnBtn3Clicked()
{
    UpdateData();

    EndDialog(2); // where 2 indicates that the btn pressed was at index 2
}
