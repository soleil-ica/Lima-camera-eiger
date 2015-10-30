object Form_Main: TForm_Main
  Left = 198
  Top = 107
  Anchors = [akLeft, akTop, akRight, akBottom]
  BorderIcons = [biSystemMenu]
  BorderStyle = bsSingle
  Caption = 'Eiger1M RESTful Simulator 1.11'
  ClientHeight = 215
  ClientWidth = 281
  Color = clBtnFace
  Constraints.MaxHeight = 328
  Constraints.MaxWidth = 287
  Constraints.MinHeight = 50
  Constraints.MinWidth = 287
  Font.Charset = DEFAULT_CHARSET
  Font.Color = clWindowText
  Font.Height = -11
  Font.Name = 'MS Sans Serif'
  Font.Style = []
  GlassFrame.Enabled = True
  OldCreateOrder = False
  OnClose = FormClose
  OnShow = FormShow
  PixelsPerInch = 96
  TextHeight = 13
  object L_Host: TLabel
    Left = 16
    Top = 0
    Width = 52
    Height = 13
    Caption = 'Running at'
  end
  object LabelFWstatus: TLabel
    Left = 24
    Top = 64
    Width = 74
    Height = 13
    Caption = 'Filewriter Status'
  end
  object LabelDETstatus: TLabel
    Left = 24
    Top = 96
    Width = 74
    Height = 13
    Caption = 'Detector Status'
  end
  object LabelCMDResponse: TLabel
    Left = 24
    Top = 128
    Width = 120
    Height = 13
    Caption = 'Command response code'
  end
  object LabelCmdDuration_arm: TLabel
    Left = 24
    Top = 160
    Width = 133
    Height = 13
    Caption = 'Command duration (ms): arm'
  end
  object LabelCmdDuration_trigger: TLabel
    Left = 24
    Top = 187
    Width = 147
    Height = 13
    Caption = 'Command Duration (ms): trigger'
  end
  object E_Host: TEdit
    Left = 16
    Top = 16
    Width = 105
    Height = 21
    ReadOnly = True
    TabOrder = 0
    Text = 'E_Host'
  end
  object Button1: TButton
    Left = 198
    Top = 8
    Width = 75
    Height = 25
    Caption = 'Console'
    TabOrder = 1
    OnClick = Button1Click
  end
  object ComboBoxFWstatus: TComboBox
    Left = 104
    Top = 61
    Width = 65
    Height = 21
    AutoComplete = False
    ItemIndex = 2
    TabOrder = 2
    Text = 'ready'
    Items.Strings = (
      'na'
      'disabled'
      'ready'
      'idle'
      'acquire'
      'error'
      'initialize'
      'configure'
      'test')
  end
  object ComboBoxDETstatus: TComboBox
    Left = 104
    Top = 93
    Width = 65
    Height = 21
    AutoComplete = False
    ItemIndex = 3
    TabOrder = 3
    Text = 'idle'
    Items.Strings = (
      'na'
      'disabled'
      'ready'
      'idle'
      'acquire'
      'error'
      'initialize'
      'configure'
      'test')
  end
  object EditCMDRespCode: TEdit
    Left = 152
    Top = 120
    Width = 33
    Height = 21
    TabOrder = 4
    Text = '200'
  end
  object EditDuration_arm: TEdit
    Left = 177
    Top = 157
    Width = 41
    Height = 21
    TabOrder = 5
    Text = '2000'
  end
  object EditDuration_trigger: TEdit
    Left = 177
    Top = 184
    Width = 41
    Height = 21
    TabOrder = 6
    Text = '1500'
  end
  object HTTPServer: TIdHTTPServer
    Active = True
    Bindings = <>
    AutoStartSession = True
    OnCommandOther = HTTPServerCommandOther
    OnCommandGet = HTTPServerCommandGet
    Left = 152
    Top = 8
  end
end
