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


class CScoreFile
{
 public:
  CScoreFile();
  
  bool create_xml_file_from_score(CScore *pscore, std::string filename);
  CScore* read_score_from_xml_file(std::string filename);

  // Debug
  bool dump_xml_file(std::string name);

 private:
  // Write
  bool add_instruments_toxml_document(TiXmlElement *pdoc, CScore *score);
  bool add_notes_toxml_document(TiXmlElement *pinstrument_element, CInstrument *pinstr);
  bool add_measures_toxml_document(TiXmlElement *pdoc, std::list<CMesure*> *pmesure_list);
  // Read
  bool get_measure(TiXmlElement *pParent, CScore *pscore);
  bool get_note_frequencies(TiXmlElement *pParent, CNote *pnote);
  bool get_notes(TiXmlElement *pParent, CInstrument *pinstr);
  bool extract_score_data_from_xml(TiXmlNode* pParent, CScore *pscore);
};
