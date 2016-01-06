/*
 Scoreview (R)
 Copyright (C) 2015 Patrick Areny
 All Rights Reserved.

 Scoreview is free software: you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation, either version 3 of the License, or
 (at your option) any later version.

 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with this program. If not, see <http://www.gnu.org/licenses/>.
*/
#include <stdio.h>
#include <assert.h>

#include <iterator>
#include <list>

#include <tinyxml.h>

#include "score.h"
#include "scorefile.h"

using namespace std;

CScoreFile::CScoreFile()
{
}

bool CScoreFile::add_measures_toxml_document(TiXmlElement *pdoc, std::list<CMesure*> *pmesure_list)
{
  std::list<CMesure*>::iterator iter;
  TiXmlElement     *pxml_measure;

  iter = pmesure_list->begin();
  while (iter != pmesure_list->end())
    {
      pxml_measure = new TiXmlElement("measure");
      pxml_measure->SetDoubleAttribute("start_timecode", (*iter)->m_start);
      pxml_measure->SetDoubleAttribute("duration", (*iter)->duration());
      pxml_measure->SetAttribute("times", (*iter)->m_times);
      pdoc->LinkEndChild(pxml_measure);
      iter++;
    }
  return true;
}

bool CScoreFile::add_notes_toxml_document(TiXmlElement *pinstrument_element, CInstrument *pinstr)
{
  CNote                     *pnote;
  std::list<t_notefreq>::iterator iter;
  TiXmlElement              *pnote_element;
  TiXmlElement              *pfreq_element;

  pnote = pinstr->get_first_note(0.);
  while (pnote != NULL)
    {
      pnote_element = new TiXmlElement("note");
      pnote_element->SetDoubleAttribute("time", pnote->m_time);
      pnote_element->SetDoubleAttribute("duration", pnote->m_duration);
      pnote_element->SetAttribute("beam", pnote->m_lconnected);
      iter = pnote->m_freq_list.begin();
      while (iter != pnote->m_freq_list.end())
	{
	  pfreq_element = new TiXmlElement("frequency");
	  pfreq_element->SetDoubleAttribute("value", (*iter).f);
	  pfreq_element->SetAttribute("string_sel", (*iter).string);
	  pnote_element->LinkEndChild(pfreq_element);
	  iter++;
	}
      pinstrument_element->LinkEndChild(pnote_element);
      pnote = pinstr->get_next_note();
    }
  return true;
}

bool CScoreFile::add_instruments_toxml_document(TiXmlElement *pdoc, CScore *pscore)
{
  CInstrument      *pins;
  TiXmlElement     *pinstrument_element;

  pins = pscore->get_first_instrument();
  while (pins != NULL)
    {
      pinstrument_element = new TiXmlElement("instrument");
      pinstrument_element->SetAttribute("name", pins->m_name);
      //pinstrument_element->SetAttribute("voice", pins->identifier());
      add_notes_toxml_document(pinstrument_element, pins);
      pdoc->LinkEndChild(pinstrument_element);
      pins = pscore->get_next_instrument();
    }
  return true;
}

bool CScoreFile::create_xml_file_from_score(CScore *pscore, string filename)
{
  TiXmlDocument     doc;
  TiXmlDeclaration *pdecl;
  TiXmlElement     *pelement;
  TiXmlElement     *pscore_element;
  TiXmlElement     *pmeasures_element;
  TiXmlElement     *pnotations_element;

  pdecl = new TiXmlDeclaration("1.0", "", "");
  doc.LinkEndChild(pdecl);

  pelement = new TiXmlElement("scoreview_file");
  pelement->SetAttribute("version", 1.0);
  doc.LinkEndChild(pelement);

  pscore_element = new TiXmlElement("score");
  doc.LinkEndChild(pscore_element);

  // Measures
  pmeasures_element = new TiXmlElement("measures");
  pscore_element->LinkEndChild(pmeasures_element);
  add_measures_toxml_document(pmeasures_element, &pscore->m_mesure_list);

  // Instruments
  add_instruments_toxml_document(pscore_element, pscore);

  // Notations, symbols like vibrato, ties...
  pnotations_element = new TiXmlElement("notations");
  pscore_element->LinkEndChild(pnotations_element);
  return doc.SaveFile(filename);
}

//------------------------------------------------------------------------------------
//
// Reading from file
//
//------------------------------------------------------------------------------------

// ----------------------------------------------------------------------
// STDOUT dump and indenting utility functions
// From the tutorial http://www.grinninglizard.com/tinyxmldocs/tutorial0.html
// ----------------------------------------------------------------------
const unsigned int NUM_INDENTS_PER_SPACE=2;

const char* getIndent(unsigned int numIndents)
{
  static const char        *pINDENT = "                                      + ";
  static const unsigned int LENGTH = strlen( pINDENT );
  unsigned int              n = numIndents*NUM_INDENTS_PER_SPACE;

  if (n > LENGTH)
    n = LENGTH;
  return &pINDENT[LENGTH - n];
}

// same as getIndent but no "+" at the end
const char* getIndentAlt( unsigned int numIndents )
{
  static const char        *pINDENT = "                                       ";
  static const unsigned int LENGTH = strlen( pINDENT );
  unsigned int              n = numIndents*NUM_INDENTS_PER_SPACE;

  if (n > LENGTH)
    n = LENGTH;
  return &pINDENT[LENGTH - n];
}

int dump_attribs_to_stdout(TiXmlElement* pElement, unsigned int indent)
{
  if (!pElement)
    return 0;

  TiXmlAttribute* pAttrib = pElement->FirstAttribute();
  int             i = 0;
  int             ival;
  double          dval;
  const char*     pIndent = getIndent(indent);

  printf("\n");
  while (pAttrib)
    {
      printf( "%s%s: value=[%s]", pIndent, pAttrib->Name(), pAttrib->Value());
      if (pAttrib->QueryIntValue(&ival) == TIXML_SUCCESS) 
	printf( " int=%d", ival);
      if (pAttrib->QueryDoubleValue(&dval) == TIXML_SUCCESS)
	printf( " d=%1.1f", dval);
      printf( "\n" );
      i++;
      pAttrib = pAttrib->Next();
    }
  return i;	
}

void dump_to_stdout(TiXmlNode* pParent, unsigned int indent = 0)
{
  if (!pParent)
    return;

  TiXmlNode* pChild;
  TiXmlText* pText;
  int        t = pParent->Type();
  printf("%s", getIndent(indent));
  int        num;

  switch (t)
    {
    case TiXmlNode::TINYXML_DOCUMENT:
      printf("Document");
      break;

    case TiXmlNode::TINYXML_ELEMENT:
      printf("Element [%s]", pParent->Value());
      num = dump_attribs_to_stdout(pParent->ToElement(), indent + 1);
      switch (num)
	{
	case 0:
	  printf(" (No attributes)");
	  break;
	case 1:
	  printf( "%s1 attribute", getIndentAlt(indent));
	  break;
	default:
	  printf( "%s%d attributes", getIndentAlt(indent), num);
	  break;
	}
      break;

    case TiXmlNode::TINYXML_COMMENT:
      printf( "Comment: [%s]", pParent->Value());
      break;

    case TiXmlNode::TINYXML_UNKNOWN:
      printf( "Unknown" );
      break;

    case TiXmlNode::TINYXML_TEXT:
      pText = pParent->ToText();
      printf("Text: [%s]", pText->Value());
      break;

    case TiXmlNode::TINYXML_DECLARATION:
      printf("Declaration");
      break;
    default:
      break;
    }
  printf( "\n" );
  for (pChild = pParent->FirstChild(); pChild != 0; pChild = pChild->NextSibling())
    {
      dump_to_stdout(pChild, indent + 1);
    }
}

bool CScoreFile::dump_xml_file(string name)
{
  TiXmlDocument doc(name);
  bool loadOkay = doc.LoadFile();
  if (loadOkay)
    {
      printf("\n%s:\n", name.c_str());
      dump_to_stdout( &doc ); // defined later in the tutorial
      return true;
    }
  else
    {
      printf("Failed to load file \"%s\"\n", name.c_str());
    }
  return false;
}

//#define VERBOSE_XML

bool CScoreFile::get_measure(TiXmlElement *pElt, CScore *pscore)
{
  string          ValueStr;
  double          timecode;
  double          duration;
  int             times;
  CMesure        *pmeasure;

  duration = timecode = 0;
  pElt->QueryDoubleAttribute("start_timecode", &timecode);
  pElt->QueryDoubleAttribute("duration", &duration);
  pElt->QueryIntAttribute("times", &times);
  pmeasure = new CMesure(timecode, timecode + duration);
  pmeasure->m_times = times;
#ifdef VERBOSE_XML
  printf("Addin a measure starting at=%f duration=%f times=%d\n", timecode, duration, times);
#endif
  pscore->m_mesure_list.push_back(pmeasure);
  return true;
}

bool CScoreFile::get_note_frequencies(TiXmlElement *pParent, CNote *pnote)
{
  TiXmlNode    *pChild;
  TiXmlElement *pElt;
  string        ValueStr;
  double        f;
  int           string_selection;
  t_notefreq    nf;

  pnote->m_freq_list.clear();
  for (pChild = pParent->FirstChild(); pChild != 0; pChild = pChild->NextSibling())
    {
      ValueStr = pChild->ValueStr();
      if (ValueStr == string("frequency"))
	{
	  f = 0;
	  pElt = pChild->ToElement();
	  pElt->QueryDoubleAttribute("value", &f);
	  pElt->QueryIntAttribute("string_sel", &string_selection);
	  nf.f = f;
	  nf.string = string_selection;
	  pnote->m_freq_list.push_back(nf);
#ifdef VERBOSE_XML
	  printf("Addinf frequency=%f and string=%d to note\n", f, nf.string);
#endif
	}
      else
	printf("Warning: unknown field in the frequency list\n");
    }
  return true;
}

bool CScoreFile::get_notes(TiXmlElement *pParent, CInstrument *pinstr)
{
  TiXmlNode      *pChild;
  TiXmlElement   *pElt;
  string          ValueStr;
  double          timecode;
  double          duration;
  int             beam;
  CNote          *pnote;

  for (pChild = pParent->FirstChild(); pChild != 0; pChild = pChild->NextSibling())
    {
      ValueStr = pChild->ValueStr();
      if (ValueStr == string("note"))
	{
	  duration = timecode = 0;
	  beam = 0;
	  pElt = pChild->ToElement();
	  pElt->QueryDoubleAttribute("time", &timecode);
	  pElt->QueryDoubleAttribute("duration", &duration);
	  pElt->QueryIntAttribute("beam", &beam);
	  pnote = new CNote(timecode, duration);
	  pnote->m_lconnected = (beam != 0);
	  get_note_frequencies(pElt, pnote);
#ifdef VERBOSE_XML
	  printf("Adding a note tcode=%f duration=%f\n", timecode, duration);
#endif
	  pinstr->add_note(pnote);
	}
      else
	printf("Warning: unknown field in the note list\n");
    }
  return true;
}

bool CScoreFile::extract_score_data_from_xml(TiXmlNode* pParent, CScore *pscore)
{
  TiXmlNode      *pChild;
#ifdef VERBOSE_XML
  TiXmlText      *pText;
#endif
  int             type;
  string          ValueStr;
  string          InstrumentName;
  //string        InstrumentVoiceName;
  CInstrument    *pinstr;
  TiXmlAttribute *pAttrib;

  if (!pParent)
    return false;
  type = pParent->Type();
  switch (type)
    {
    case TiXmlNode::TINYXML_ELEMENT:
#ifdef VERBOSE_XML
      printf("Element [%s]", pParent->Value());
#endif
      ValueStr = pParent->ValueStr();
      if (ValueStr == string("measures"))
	{
	}
      if (ValueStr == string("measure"))
	{
	  get_measure(pParent->ToElement(), pscore);
	}
      if (ValueStr == string("instrument"))
	{
	  pAttrib = pParent->ToElement()->FirstAttribute();
	  while (pAttrib != NULL)
	    {
	      if (string(pAttrib->Name()) == string("name"))
		{
		  InstrumentName = string(pAttrib->Value());
		  break;
		}
	      /*
	      if (string(pAttrib->Name()) == string("voice"))
		{
		  InstrumentVoiceName = string(pAttrib->Value());
		  break;
		}
		*/
	      pAttrib = pAttrib->Next();
	    }
#ifdef VERBOSE_XML
	  printf("creating an istrument of name=%s\n", InstrumentName.c_str());
#endif
	  pinstr = new CInstrument(InstrumentName);
	  get_notes(pParent->ToElement(), pinstr);
	  pscore->add_instrument(pinstr);
	  // Branch out of the notes section
	  return true;
	}
      break;
#ifdef VERBOSE_XML
    case TiXmlNode::TINYXML_DOCUMENT:
      printf("Document");
      break;
    case TiXmlNode::TINYXML_COMMENT:
      printf( "Comment: [%s]", pParent->Value());
      break;
    case TiXmlNode::TINYXML_UNKNOWN:
      printf( "Unknown" );
      break;
    case TiXmlNode::TINYXML_TEXT:
      pText = pParent->ToText();
      printf("Text: [%s]", pText->Value());
      break;
    case TiXmlNode::TINYXML_DECLARATION: // like xml=1.0
      printf("Declaration");
      break;
#endif //VERBOSE_XML
    default:
      break;
    }
#ifdef VERBOSE_XML
  printf( "\n" );
#endif
  for (pChild = pParent->FirstChild(); pChild != 0; pChild = pChild->NextSibling())
    {
      extract_score_data_from_xml(pChild->ToElement(), pscore);
    }
  return true;
}

CScore* CScoreFile::read_score_from_xml_file(string filename)
{
  CScore *pretscore;
  string instrument_name;
  TiXmlDocument *pdoc;
  bool bload;

  try
    {
      printf("loading the score xml file.\n");
      pdoc = new TiXmlDocument(filename);
      bload = pdoc->LoadFile();
      if (!bload)
	{
	  delete pdoc;
	  throw 1;
	}
      pretscore = new CScore(); // Empty score
      if (!extract_score_data_from_xml(pdoc, pretscore))
	{
	  delete pretscore;
	  throw 2;
	}
      delete pdoc;
    }
  catch (int e)
    {
      switch (e)
	{
	case 2:
	  break;
	case 1:
	  break;
	default:
	  break;
	}
      return NULL;
    }
  return pretscore;
}

