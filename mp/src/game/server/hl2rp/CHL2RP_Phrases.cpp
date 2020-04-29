#include "cbase.h"
#include "CHL2RP_Phrases.h"
#include "CHL2RP.h"
#include "CHL2RP_Player.h"
#include "CHL2RP_PlayerDialog.h"
#include "language.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

// NOTE: Add phrase color codes ONLY inside each CPhraseParam's format.
// This is to apply them automatically on all languages.
// Never add them on the default translation strings!

#define PHRASE_POSITION_FORMAT(POSITION) "{" #POSITION "}"

const int MAX_PHRASE_LANGUAGE_SHORT_NAME_LENGTH = 64; // Should be more than enough

extern const char DEFAULT_PHRASE_LANGUAGE[] = "english";

extern const char WELCOME_CENTER_HUD_HEADER[] = "Welcome center text";
extern const char MAIN_HUD_HEADER[] = "Main hud";
extern const char REGION_CHAT_HEADER[] = "Region chat";
extern const char REGION_HUD_HEADER[] = "Region hud";
extern const char EXTENDED_REGION_HUD_HEADER[] = "Extended region hud";
extern const char WEAPONS_AND_CLIPS_LOADED_HEADER[] = "Weapons loaded";
extern const char RESERVE_AMMO_LOADED_HEADER[] = "Reserve ammo loaded";
extern const char PARENT_MENU_ITEM_HEADER[] = "Back menu item";
extern const char NEXT_MENU_ITEM_HEADER[] = "Next menu item";
extern const char PREVIOUS_MENU_ITEM_HEADER[] = "Prev. menu item";
extern const char MAIN_MENU_TITLE_HEADER[] = "Main menu title";
extern const char MAIN_MENU_MSG_HEADER[] = "Main menu msg";
extern const char PHRASE_LANGUAGE_MENU_TITLE_HEADER[] = "Phrase language title";
extern const char PHRASE_LANGUAGE_MENU_MSG_HEADER[] = "Phrase language msg";
extern const char PHRASE_HEADER_MENU_TITLE_HEADER[] = "Phrase header title";
extern const char PHRASE_HEADER_MENU_MSG_HEADER[] = "Phrase header msg";
extern const char PHRASE_TRANSLATION_ENTRY_BOX_TITLE_HEADER[] = "Phrase translation title";
extern const char PHRASE_TRANSLATION_ENTRY_BOX_MSG_HEADER[] = "Phrase translation msg";
extern const char PHRASE_TRANSLATION_SAVE_MENU_TITLE_HEADER[] = "Phrase save title";
extern const char PHRASE_TRANSLATION_SAVE_MENU_MSG_HEADER[] = "Phrase save header";
extern const char ZONE_MENU_MSG_HEADER[] = "Zone menu header";
extern const char SAVE_MENU_ITEM_HEADER[] = "Save";
extern const char DISCARD_MENU_ITEM_HEADER[] = "Discard";
extern const char ADMIN_MENU_ITEM_HEADER[] = "Admin menu";
extern const char ADMIN_MENU_TITLE_HEADER[] = "Admin menu title";
extern const char ADMIN_MENU_MSG_HEADER[] = "Admin menu msg";
extern const char TRANSLATE_PHRASE_MENU_ITEM_HEADER[] = "Translate phrase";
extern const char ADD_ZONE_MENU_ITEM_HEADER[] = "Add zone";
extern const char MANAGE_DISPENSERS_MENU_ITEM_HEADER[] = "Manage dispensers";
extern const char ADD_POINT_MENU_ITEM_HEADER[] = "Add point";
extern const char SET_POINT_MENU_ITEM_HEADER[] = "Set point";
extern const char CREATE_DISPENSER_MENU_ITEM_HEADER[] = "Create dispenser";
extern const char MOVE_CURRENT_DISPENSER_MENU_ITEM_HEADER[] = "Move cur. dispenser";
extern const char SAVE_CURRENT_DISPENSER_MENU_ITEM_HEADER[] = "Save cur. dispenser";
extern const char REMOVE_CURRENT_DISPENSER_MENU_ITEM_HEADER[] = "Rem. cur. dispenser";
extern const char SAVE_AIMED_DISPENSER_MENU_ITEM_HEADER[] = "Save aim dispenser";
extern const char REMOVE_AIMED_DISPENSER_MENU_ITEM_HEADER[] = "Rem. aim dispenser";
extern const char DEFAULT_PHRASE_TRANSLATION_HINT_HEADER[] = "Default translation";

#ifdef HL2DM_RP
extern const char SPECIAL_WEAPONS_MENU_ITEM_HEADER[] = "Special weapons";
extern const char SPECIAL_WEAPONS_MENU_MSG_HEADER[] = "Special weapons msg";
extern const char WEAPON_CITIZENPACKAGE_MENU_ITEM_HEADER[] = "Rations";
extern const char WEAPON_CITIZENSUITCASE_MENU_ITEM_HEADER[] = "Suitcase";
extern const char WEAPON_MOLOTOV_MENU_ITEM_HEADER[] = "Molotov";
extern const char MANAGE_COMPAT_MODELS_MENU_ITEM_HEADER[] = "Manage compats";
extern const char MANAGE_COMPAT_MODELS_MENU_TITLE_HEADER[] = "Manage compats title";
extern const char MANAGE_COMPAT_MODELS_MENU_MSG_HEADER[] = "Manage compats msg";
extern const char ADD_COMPAT_MODEL_MENU_ITEM_HEADER[] = "Add model";
extern const char REMOVE_COMPAT_MODEL_MENU_ITEM_HEADER[] = "Remove model";
extern const char ADD_COMPAT_MODEL_TYPE_MENU_TITLE_HEADER[] = "Compat type title";
extern const char ADD_COMPAT_MODEL_TYPE_MENU_MSG_HEADER[] = "Compat type msg";
extern const char ADD_COMPAT_MODEL_ID_ENTRY_BOX_TITLE_HEADER[] = "Compat id title";
extern const char ADD_COMPAT_MODEL_ID_ENTRY_BOX_MSG_HEADER[] = "Compat id msg";
extern const char ADD_COMPAT_MODEL_PATH_ENTRY_BOX_TITLE_HEADER[] = "Compat path title";
extern const char ADD_COMPAT_MODEL_PATH_ENTRY_BOX_MSG_HEADER[] = "Compat path msg";
extern const char SAVE_COMPAT_MODEL_MENU_TITLE_HEADER[] = "Save compat title";
extern const char SAVE_COMPAT_MODEL_MENU_MSG_HEADER[] = "Save compat msg";
extern const char REMOVE_COMPAT_MODELS_MENU_TITLE_HEADER[] = "Remove compats title";
extern const char REMOVE_COMPAT_MODELS_MENU_MSG_HEADER[] = "Remove compats msg";
#endif

enum PhrasePositionFormatSearchState
{
	SEARCHING_OPEN_CURLY_BRACKET,
	SEARCHING_DIGIT_OR_CLOSE_CURLY_BRACKET
};

CPhraseParam::CPhraseParam(int iValue, const char *pszValueFormat) : m_Type(PHRASE_PARAM_INT), m_pszValueFormat(pszValueFormat)
{
	m_iValue = iValue;
}

CPhraseParam::CPhraseParam(float flValue, const char *pszValueFormat) : m_Type(PHRASE_PARAM_FLOAT), m_pszValueFormat(pszValueFormat)
{
	m_flValue = flValue;
}

CPhraseParam::CPhraseParam(const char *pszValue, const char *pszValueFormat) : m_Type(PHRASE_PARAM_STRING), m_pszValueFormat(pszValueFormat)
{
	m_pszValue = pszValue;
}

void CHL2RP_Phrases::Init()
{
	CHL2RP::s_pPhrases = new KeyValues("Phrases");
	KeyValues *defaultLanguageKV = CHL2RP::s_pPhrases->CreateNewKey();
	defaultLanguageKV->SetName(DEFAULT_PHRASE_LANGUAGE);
	AddPhrase(defaultLanguageKV, WELCOME_CENTER_HUD_HEADER,
		"Welcome to Half-Life 2 RolePlay\n· Open your Main Menu any time by\nholding +USE key for {1} second(s)"
		"\n· Go in search of Civil Protection food\npackages, found at Ration Dispensers\n· And interact with other Citizens freely");
	AddPhrase(defaultLanguageKV, MAIN_HUD_HEADER,
		"Play time: " PHRASE_POSITION_FORMAT(1) "h " PHRASE_POSITION_FORMAT(2) "min " PHRASE_POSITION_FORMAT(3) "s"
		"\nRations: " PHRASE_POSITION_FORMAT(4) "\nCrime: " PHRASE_POSITION_FORMAT(5));
	AddPhrase(defaultLanguageKV, REGION_CHAT_HEADER, "(Region) " PHRASE_POSITION_FORMAT(1) " : " PHRASE_POSITION_FORMAT(2));
	AddPhrase(defaultLanguageKV, REGION_HUD_HEADER, "Region-VoiceList: " PHRASE_POSITION_FORMAT(1));
	AddPhrase(defaultLanguageKV, EXTENDED_REGION_HUD_HEADER, "Region-VoiceList (" PHRASE_POSITION_FORMAT(1) "): " PHRASE_POSITION_FORMAT(2));
	AddPhrase(defaultLanguageKV, WEAPONS_AND_CLIPS_LOADED_HEADER, "Your weapons have been loaded and clips added up to your current");
	AddPhrase(defaultLanguageKV, RESERVE_AMMO_LOADED_HEADER, "Your reserve ammo has been loaded and added up to your current");
	AddPhrase(defaultLanguageKV, PARENT_MENU_ITEM_HEADER, PHRASE_POSITION_FORMAT(1) ". [Back]");
	AddPhrase(defaultLanguageKV, NEXT_MENU_ITEM_HEADER, PHRASE_POSITION_FORMAT(1) ". [Next]");
	AddPhrase(defaultLanguageKV, PREVIOUS_MENU_ITEM_HEADER, PHRASE_POSITION_FORMAT(1) ". [Previous]");
	AddPhrase(defaultLanguageKV, MAIN_MENU_TITLE_HEADER, "Main menu: press ESC");
	AddPhrase(defaultLanguageKV, MAIN_MENU_MSG_HEADER, "Here is your main\nmenu with everything");
	AddPhrase(defaultLanguageKV, PHRASE_LANGUAGE_MENU_TITLE_HEADER, "Language menu");
	AddPhrase(defaultLanguageKV, PHRASE_LANGUAGE_MENU_MSG_HEADER, "First choose an available language");
	AddPhrase(defaultLanguageKV, PHRASE_HEADER_MENU_TITLE_HEADER, "Phrase header menu");
	AddPhrase(defaultLanguageKV, PHRASE_HEADER_MENU_MSG_HEADER, "Now choose a phrase header\nand view on console the\nlanguage translation.\n"
		"Each phrase has values\nthat server fills later.\n"
		"You reference them as\norder numbers between\nbrackets, like this:\nName: " PHRASE_POSITION_FORMAT(1) ". Surname: " PHRASE_POSITION_FORMAT(2) ".\n"
		"You don't have to insert\ncolor codes, server places\nfixed ones already smartly.\n"
		"You can also enter phrase\nwith entryboxtext <phrase> command");
	AddPhrase(defaultLanguageKV, PHRASE_TRANSLATION_ENTRY_BOX_TITLE_HEADER, "Translation entry box");
	AddPhrase(defaultLanguageKV, PHRASE_TRANSLATION_ENTRY_BOX_MSG_HEADER, "Enter the new translation");
	AddPhrase(defaultLanguageKV, PHRASE_TRANSLATION_SAVE_MENU_TITLE_HEADER, "Translation save menu");
	AddPhrase(defaultLanguageKV, PHRASE_TRANSLATION_SAVE_MENU_MSG_HEADER,
		"Are you sure that\nyou want to replace\ncurrent translation with:\n\n\"" PHRASE_POSITION_FORMAT(1) "\"?\n\n");
	AddPhrase(defaultLanguageKV, ZONE_MENU_MSG_HEADER, "To raise and lower height,\nbind +zone_height_up and\n+zone_height_down to a key\n"
		"\nExample (arrow keys):\n\nbind UPARROW +zone_height_up\nbind DOWNARROW +zone_height_down");
	AddPhrase(defaultLanguageKV, SAVE_MENU_ITEM_HEADER, "Save");
	AddPhrase(defaultLanguageKV, DISCARD_MENU_ITEM_HEADER, "Discard");
	AddPhrase(defaultLanguageKV, ADMIN_MENU_ITEM_HEADER, "Administration");
	AddPhrase(defaultLanguageKV, ADMIN_MENU_TITLE_HEADER, "Administration menu");
	AddPhrase(defaultLanguageKV, ADMIN_MENU_MSG_HEADER, "Here you can perform\nmany administrative tasks");
	AddPhrase(defaultLanguageKV, TRANSLATE_PHRASE_MENU_ITEM_HEADER, "Translate phrase");
	AddPhrase(defaultLanguageKV, ADD_ZONE_MENU_ITEM_HEADER, "Add zone");
	AddPhrase(defaultLanguageKV, MANAGE_DISPENSERS_MENU_ITEM_HEADER, "Manage dispensers");
	AddPhrase(defaultLanguageKV, ADD_POINT_MENU_ITEM_HEADER, "Add point " PHRASE_POSITION_FORMAT(1));
	AddPhrase(defaultLanguageKV, SET_POINT_MENU_ITEM_HEADER, "Set point " PHRASE_POSITION_FORMAT(1));
	AddPhrase(defaultLanguageKV, CREATE_DISPENSER_MENU_ITEM_HEADER, "Create dispenser");
	AddPhrase(defaultLanguageKV, MOVE_CURRENT_DISPENSER_MENU_ITEM_HEADER, "Move current");
	AddPhrase(defaultLanguageKV, SAVE_CURRENT_DISPENSER_MENU_ITEM_HEADER, "Save current");
	AddPhrase(defaultLanguageKV, REMOVE_CURRENT_DISPENSER_MENU_ITEM_HEADER, "Remove current");
	AddPhrase(defaultLanguageKV, SAVE_AIMED_DISPENSER_MENU_ITEM_HEADER, "Save aimed");
	AddPhrase(defaultLanguageKV, REMOVE_AIMED_DISPENSER_MENU_ITEM_HEADER, "Remove aimed");
	AddFormattedPhrase(defaultLanguageKV, DEFAULT_PHRASE_TRANSLATION_HINT_HEADER,
		"\n\nDefault translation [%s] of \"" PHRASE_POSITION_FORMAT(1) "\":\n\n", DEFAULT_PHRASE_LANGUAGE);

#ifdef HL2DM_RP
	AddPhrase(defaultLanguageKV, SPECIAL_WEAPONS_MENU_ITEM_HEADER, "Special weapons");
	AddPhrase(defaultLanguageKV, SPECIAL_WEAPONS_MENU_MSG_HEADER, "Here you can switch between\nspecial hand weapons or items\n"
		"that reuse the same slot,\nbeing unselectable from controls");
	AddPhrase(defaultLanguageKV, WEAPON_CITIZENPACKAGE_MENU_ITEM_HEADER, "Rations");
	AddPhrase(defaultLanguageKV, WEAPON_CITIZENSUITCASE_MENU_ITEM_HEADER, "Suitcase");
	AddPhrase(defaultLanguageKV, WEAPON_MOLOTOV_MENU_ITEM_HEADER, "Molotov");
	AddPhrase(defaultLanguageKV, MANAGE_COMPAT_MODELS_MENU_ITEM_HEADER, "Manage compats");
	AddPhrase(defaultLanguageKV, MANAGE_COMPAT_MODELS_MENU_TITLE_HEADER, "Compatible HL2SP models");
	AddPhrase(defaultLanguageKV, MANAGE_COMPAT_MODELS_MENU_MSG_HEADER, "Here you can manage character\nmodels with HL2SP animations\n"
		"using different paths than\nHL2MP ones to avoid conflicts");
	AddPhrase(defaultLanguageKV, ADD_COMPAT_MODEL_MENU_ITEM_HEADER, "Add model");
	AddPhrase(defaultLanguageKV, REMOVE_COMPAT_MODEL_MENU_ITEM_HEADER, "Remove model");
	AddPhrase(defaultLanguageKV, ADD_COMPAT_MODEL_TYPE_MENU_TITLE_HEADER, "Compatible HL2SP model type");
	AddPhrase(defaultLanguageKV, ADD_COMPAT_MODEL_TYPE_MENU_MSG_HEADER, "Choose which team the\nmodel will be assigned to");
	AddPhrase(defaultLanguageKV, ADD_COMPAT_MODEL_ID_ENTRY_BOX_TITLE_HEADER, "Compatible HL2SP model id");
	AddPhrase(defaultLanguageKV, ADD_COMPAT_MODEL_ID_ENTRY_BOX_MSG_HEADER, "Insert a name that will uniquely\nidentify the compatible model");
	AddPhrase(defaultLanguageKV, ADD_COMPAT_MODEL_PATH_ENTRY_BOX_TITLE_HEADER, "Compatible HL2SP model path");
	AddPhrase(defaultLanguageKV, ADD_COMPAT_MODEL_PATH_ENTRY_BOX_MSG_HEADER, "Finally set the model\npath from filesystem");
	AddPhrase(defaultLanguageKV, SAVE_COMPAT_MODEL_MENU_TITLE_HEADER, "Save compatible HL2SP model");
	AddPhrase(defaultLanguageKV, SAVE_COMPAT_MODEL_MENU_MSG_HEADER, "Here you can save the\nmodel if you are sure,\nor discard its saving");
	AddPhrase(defaultLanguageKV, REMOVE_COMPAT_MODELS_MENU_TITLE_HEADER, "Remove compat models");
	AddPhrase(defaultLanguageKV, REMOVE_COMPAT_MODELS_MENU_MSG_HEADER, "Here you can remove any\nstored compatible model");
#endif
}

// Adds a phrase that is not suppossed to exist without pre searching it, for efficiency:
void CHL2RP_Phrases::AddPhrase(KeyValues *pLanguageKV, const char *pszHeader, const char *pszTranslation)
{
	Assert(pLanguageKV != NULL);
	KeyValues *headerKV = pLanguageKV->CreateNewKey();
	headerKV->SetName(pszHeader);
	headerKV->SetStringValue(pszTranslation);
}

void CHL2RP_Phrases::AddFormattedPhrase(KeyValues *pLanguageKV, const char *pszHeader, const char *pszTranslationFormat, ...)
{
	va_list phraseFormatArgs;
	va_start(phraseFormatArgs, pszTranslationFormat);
	char szFinalTranslation[MAX_PHRASE_TRANSLATION_LENGTH];
	Q_vsnprintf(szFinalTranslation, sizeof(szFinalTranslation), pszTranslationFormat, phraseFormatArgs);
	AddPhrase(pLanguageKV, pszHeader, szFinalTranslation);
	va_end(phraseFormatArgs);
}

void CHL2RP_Phrases::LoadForPlayer(const CHL2RP_Player *pPlayer, char *psDest, int iMaxlen,
	const char *pszHeader, const char *psPrefix, const CPhraseParam paramsBuff[], int iParamCount)
{
	Assert(pPlayer != NULL);
	Assert((psDest != NULL) && (0 < iMaxlen));
	Assert(pszHeader != NULL);
	Assert(paramsBuff != NULL);
	KeyValues *languageKv = CHL2RP::s_pPhrases->FindKey(engine->GetClientConVarValue(pPlayer->entindex(), "cl_language"));
	const char *translation;
	int numDestCharsWritten = 0;
	char positionText[4] = ""; // Enough size (0 - 100)

	if (languageKv == NULL || (translation = languageKv->GetString(pszHeader, NULL)) == NULL)
	{
		translation = CHL2RP::s_pPhrases->FindKey(DEFAULT_PHRASE_LANGUAGE)->GetString(pszHeader);
	}

	if (psPrefix != NULL)
	{
		numDestCharsWritten += Q_snprintf(psDest, iMaxlen, psPrefix);
	}

	if (iParamCount > 0)
	{
		PhrasePositionFormatSearchState state = SEARCHING_OPEN_CURLY_BRACKET;

		// Start copying translation into dest step by step, and formatting parameters when necessary...
		for (int i = 0, numDestCharsWrittenPreBracket = 0, numDigitsWritten = 0;
			translation[i] != '\0' && numDestCharsWritten < iMaxlen - 1; i++)
		{
			switch (state)
			{
			case SEARCHING_OPEN_CURLY_BRACKET:
			{
				if (translation[i] == '{')
				{
					numDestCharsWrittenPreBracket = numDestCharsWritten;
					state = SEARCHING_DIGIT_OR_CLOSE_CURLY_BRACKET;
				}

				break;
			}
			case SEARCHING_DIGIT_OR_CLOSE_CURLY_BRACKET:
			{
				if (V_isdigit(translation[i]) && numDigitsWritten < sizeof positionText)
				{
					positionText[numDigitsWritten++] = translation[i];
				}
				else
				{
					int8 paramIndex = atoi(positionText) - 1;
					numDigitsWritten = 0; // Reset it for filling further digits correctly
					state = SEARCHING_OPEN_CURLY_BRACKET;

					if (translation[i] == '}' && 0 <= paramIndex && paramIndex < iParamCount)
					{
						// Current position format search succeeded, format the parameter and append it to dest
						int paramFormatLen = 0;

						switch (paramsBuff[paramIndex].m_Type)
						{
						case CPhraseParam::PHRASE_PARAM_INT:
						{
							paramFormatLen = Q_snprintf(&psDest[numDestCharsWrittenPreBracket], iMaxlen - numDestCharsWrittenPreBracket,
								paramsBuff[paramIndex].m_pszValueFormat, paramsBuff[paramIndex].m_iValue);
							break;
						}
						case CPhraseParam::PHRASE_PARAM_FLOAT:
						{
							paramFormatLen = Q_snprintf(&psDest[numDestCharsWrittenPreBracket], iMaxlen - numDestCharsWrittenPreBracket,
								paramsBuff[paramIndex].m_pszValueFormat, paramsBuff[paramIndex].m_flValue);
							break;
						}
						case CPhraseParam::PHRASE_PARAM_STRING:
						{
							paramFormatLen = Q_snprintf(&psDest[numDestCharsWrittenPreBracket], iMaxlen - numDestCharsWrittenPreBracket,
								paramsBuff[paramIndex].m_pszValueFormat, paramsBuff[paramIndex].m_pszValue);
							break;
						}
						}

						numDestCharsWritten = numDestCharsWrittenPreBracket + paramFormatLen;
						continue; // Prevent copying the close curly bracket
					}
				}

				break;
			}
			}

			// Ensure that chars are copied before loop may stops
			psDest[numDestCharsWritten++] = translation[i];
		}

		// Ensure dest buffer is NULL terminated, at the correct limit
		psDest[numDestCharsWritten] = '\0';
	}
	else // Attempt to optimize the copy against the cost of position format search
	{
		Q_strncpy(&psDest[numDestCharsWritten], translation, iMaxlen - numDestCharsWritten);
	}
}

CSetUpPhrasesTxn::CSetUpPhrasesTxn()
{
	AddFormattedQuery("CREATE TABLE IF NOT EXISTS Phrase "
		"(id VARCHAR (%i) NOT NULL, language VARCHAR (%i) NOT NULL, "
		"translation VARCHAR (%i) NOT NULL, PRIMARY KEY (id, language));",
		MAX_PHRASE_HEADER_LENGTH, MAX_PHRASE_LANGUAGE_SHORT_NAME_LENGTH, MAX_PHRASE_TRANSLATION_LENGTH);
}

bool CSetUpPhrasesTxn::ShouldUsePreparedStatements() const
{
	return false;
}

CLoadPhrasesTxn::CLoadPhrasesTxn() : CAsyncSQLTxn("SELECT * FROM Phrase;")
{

}

bool CLoadPhrasesTxn::ShouldUsePreparedStatements() const
{
	return false;
}

void CLoadPhrasesTxn::HandleQueryResults() const
{
	while (CHL2RP::s_pSQL->FetchNextRow())
	{
		char header[MAX_PHRASE_HEADER_LENGTH], language[MENU_ITEM_LENGTH], translation[MAX_PHRASE_TRANSLATION_LENGTH];
		CHL2RP::s_pSQL->FetchString("language", language, sizeof language);
		CHL2RP::s_pSQL->FetchString("id", header, sizeof header);
		CHL2RP::s_pSQL->FetchString("translation", translation, sizeof translation);

		FOR_EACH_LANGUAGE(eLang)
		{
			// Only add if language is recognized and header exists on KeyValues:
			if (!Q_strcmp(GetLanguageShortName((ELanguage)eLang), language)
				&& CHL2RP::s_pPhrases->FindKey(DEFAULT_PHRASE_LANGUAGE)->FindKey(header))
			{
				KeyValues *languageKv = CHL2RP::s_pPhrases->FindKey(language, true);
				languageKv->SetString(header, translation);
				break;
			}
		}
	}
}

CUpsertPhraseTxn::CUpsertPhraseTxn(const char *phraseLanguageShortName, const char *phraseHeader, const char *phraseTranslation) :
m_pszPhraseLanguageShortName(phraseLanguageShortName)
{
	Q_strncpy(m_szPhraseHeader, phraseHeader, sizeof(m_szPhraseHeader));
	Q_strncpy(m_szPhraseTranslation, phraseTranslation, sizeof(m_szPhraseTranslation));
}

void CUpsertPhraseTxn::BuildSQLiteQueries()
{
	AddQuery("UPDATE Phrase SET translation = ? WHERE id = ? AND language = ?;");
	AddQuery("INSERT OR IGNORE INTO Phrase (id, language, translation) VALUES (?, ?, ?);");
}

void CUpsertPhraseTxn::BuildMySQLQueries()
{
	AddQuery("INSERT INTO Phrase (id, language, translation) VALUES (?, ?, ?) ON DUPLICATE KEY UPDATE translation = ?;");
}

void CUpsertPhraseTxn::BindSQLiteParameters(int iQueryId, const sqlite3_stmt *pStmt) const
{
	if (!iQueryId)
	{
		CHL2RP::s_pSQL->BindString(pStmt, m_szPhraseTranslation, 1);
		CHL2RP::s_pSQL->BindString(pStmt, m_szPhraseHeader, 2);
		CHL2RP::s_pSQL->BindString(pStmt, m_pszPhraseLanguageShortName, 3);
	}
	else
	{
		CHL2RP::s_pSQL->BindString(pStmt, m_szPhraseHeader, 1);
		CHL2RP::s_pSQL->BindString(pStmt, m_pszPhraseLanguageShortName, 2);
		CHL2RP::s_pSQL->BindString(pStmt, m_szPhraseTranslation, 3);
	}
}

void CUpsertPhraseTxn::BindMySQLParameters(int iQueryId, const PreparedStatement *pStmt) const
{
	CHL2RP::s_pSQL->BindString(pStmt, m_szPhraseHeader, 1);
	CHL2RP::s_pSQL->BindString(pStmt, m_pszPhraseLanguageShortName, 2);
	CHL2RP::s_pSQL->BindString(pStmt, m_szPhraseTranslation, 3, 4);
}

bool CUpsertPhraseTxn::ShouldBeReplacedBy(const BaseClass &txn) const
{
	ThisClass *pThisClassOtherTxn = txn.ToTxnClass(this);
	return ((pThisClassOtherTxn != NULL) && (pThisClassOtherTxn->m_pszPhraseLanguageShortName == m_pszPhraseLanguageShortName)
		&& !Q_strcmp(pThisClassOtherTxn->m_szPhraseHeader, m_szPhraseHeader));
}
