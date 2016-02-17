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

#define STORAGE_DIALOG_CODE_NAME          "save_open_dialog"
#define ADDINSTRUMENT_DIALOG_CODENAME     "add_instrument_dialog"
#define PRACTICE_DIALOG_CODENAME          "practice_dialog"
#define CONFIG_DIALOG_CODENAME            "config_dialog"

#define MESSAGE_CONTENT_SIZE 4096

#define NOTES_PER_NOTE_TRANSFER 40

enum edialogs_codes
  {
    unknown_dialog,
    load_save_dialog,
    add_instrument_dialog,
    practice_dialog,
    config_dialog
  };

enum eWiremessages
  {
    network_message_void,
    //----------------------------------------------------------------------
    // Messages sent to dialog boxes
    //----------------------------------------------------------------------
    network_message_close = 7530,
    network_message_remadd_note,
    network_message_remadd_measure,
    network_message_practice,
    network_message_instrument_list,
    network_message_score_transfer,
    network_message_note_transfer,
    network_message_note_highlight,
    //----------------------------------------------------------------------
    // bidirectional
    //----------------------------------------------------------------------
    network_message_pa_dev_selection = 8700,
    network_message_configuration,
    //----------------------------------------------------------------------
    // Messages sent by dialog boxes
    //----------------------------------------------------------------------
    network_message_closed_dialog = 5490,
    network_message_dialog_opened,
    network_message_file,
    network_message_remadd_instrument
  };

//----------------------------------------------------------------------
// Internal events messages from the main app to the dialogs thread
//----------------------------------------------------------------------

enum edialog_message
  {
    message_open_storage_dialog = 8480,
    message_open_addinstrument_dialog,
    message_open_practice_dialog,
    message_open_config_dialog,
    message_send_instrument_list,
    message_close_dialogs,
    message_send_practice,
    message_send_remadd_note,
    message_send_remadd_measure,
    message_quit_its_over,
    message_send_configuration,
    message_send_pa_devices_list,
    message_score_transfer,
    message_note_transfer,
    message_note_highlight,
    message_close_storage_dialog,
    message_close_addinstrument_dialog,
    message_close_practice_dialog,
    message_close_config_dialog
  };

