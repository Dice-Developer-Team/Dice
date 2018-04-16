#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <windows.h>
#include <windows.h>
#include <stdio.h>
#include <conio.h>
#include <io.h>
#include <shlwapi.h>

#include <QFileDialog>
#include <QMimeData>
#include <QDragEnterEvent>

#include "zip.h"
#include "unzip.h"
#include "rapidxml_print.hpp"

#pragma comment(lib,"shlwapi.lib") 

#define ENABLE_EXPORT_SEPARATELY 0
#define ENABLE_IMPORT 0

extern char* drag_file_path;

using namespace rapidxml;

wchar_t* GetErrorTips();
wchar_t* Utf8ToWchar(char* utf8_str);
char* WcharToUtf8(wchar_t* unicode_str);
int GetPropertyFromXlsx(wchar_t* file_path, Ui::MainWindow *ui);
int SetPropertyToXlsx(wchar_t* file_path, wchar_t* templet_string, Ui::MainWindow *ui);

wchar_t name[32];

const char* name_r = "D3";

char property_list_1[][5][16] = {
{ "R3","T3","V3","V4","" },
{ "R5","T5","V5","V6","" },
{ "R7","T7","V7","V8","" },
{ "X3","Z3","AB3","AB4","" },
{ "X5","Z5","AB5","AB6","" },
{ "X7","Z7","AB7","AB8","" },
{ "AD3","AF3","AH3","AH4","" },
{ "AD5","AF5","AH5","AH6","" },

{ "B10","F10","","","" },
{ "K10","O10","","","" },
{ "S10","W10","","","" },
{ "AA10","AE10","","","" },

{ "C15","Q15","S15","U15","K15" },
{ "C16","Q16","S16","U16","K16" },
{ "C17","Q17","S17","U17","K17" },
{ "C18","Q18","S18","U18","K18" },
{ "E19","Q19","S19","U19","K19" },
{ "E20","Q20","S20","U20","K20" },
{ "E21","Q21","S21","U21","K21" },
{ "C22","Q22","S22","U22","K22" },
{ "C23","Q23","S23","U23","K23" },
{ "C24","Q24","S24","U24","K24" },
{ "C25","Q25","S25","U25","K25" },
{ "C26","Q26","S26","U26","K26" },
{ "C27","Q27","S27","U27","K27" },
{ "C28","Q28","S28","U28","K28" },
{ "C29","Q29","S29","U29","K29" },
{ "C30","Q30","S30","U30","K30" },
{ "C31","Q31","S31","U31","K31" },
{ "C32","Q32","S32","U32","K32" },
{ "E33","Q33","S33","U33","K33" },
{ "E34","Q34","S34","U34","K34" },
{ "E35","Q35","S35","U35","K35" },
{ "E36","Q36","S36","U36","K36" },
{ "E37","Q37","S37","U37","K37" },
{ "E38","Q38","S38","U38","K38" },
{ "C39","Q39","S39","U39","K39" },
{ "C40","Q40","S40","U40","K40" },
{ "C41","Q41","S41","U41","K41" },
{ "C42","Q42","S42","U42","K42" },
{ "E43","Q43","S43","U43","K43" },
{ "E44","Q44","S44","U44","K44" },
{ "E45","Q45","S45","U45","K45" },
{ "E46","Q46","S46","U46","K56" },

{ "X15","AK15","AM15","AO15","AE15" },
{ "X16","AK16","AM16","AO16","AE16" },
{ "X17","AK17","AM17","AO17","AE17" },
{ "X18","AK18","AM18","AO18","AE18" },
{ "X19","AK19","AM19","AO19","AE19" },
{ "X20","AK20","AM20","AO20","AE20" },
{ "X21","AK21","AM21","AO21","AE21" },
{ "X22","AK22","AM22","AO22","AE22" },
{ "X23","AK23","AM23","AO23","AE23" },
{ "X24","AK24","AM24","AO24","AE24" },
{ "X25","AK25","AM25","AO25","AE25" },
{ "Z26","AK26","AM26","AO26","AE26" },
{ "X27","AK27","AM27","AO27","AE27" },
{ "X28","AK28","AM28","AO28","AE28" },
{ "X29","AK29","AM29","AO29","AE29" },
{ "Z30","AK30","AM30","AO30","AE30" },
{ "Z31","AK31","AM31","AO31","AE31" },
{ "Z32","AK32","AM32","AO32","AE32" },
{ "X33","AK33","AM33","AO33","AE33" },
{ "X34","AK34","AM34","AO34","AE34" },
{ "X35","AK35","AM35","AO35","AE35" },
{ "Z36","AK36","AM36","AO36","AE36" },
{ "Z37","AK37","AM37","AO37","AE37" },
{ "X38","AK38","AM38","AO38","AE38" },
{ "X39","AK39","AM39","AO39","AE39" },
{ "X40","AK40","AM40","AO40","AE40" },
{ "Z41","AK41","AM41","AO41","AE41" },
{ "Z42","AK42","AM42","AO42","AE42" },
{ "X43","AK43","AM43","AO43","AE43" },
{ "X44","AK44","AM44","AO44","AE44" },
{ "X45","AK45","AM45","AO45","AE45" },
{ "X46","AK46","AM15","AO15","AE15" }
};

char property_list_2[][5][16] = {
{ "Q3","S3","U3","U4","" },
{ "Q5","S5","U5","U6","" },
{ "Q7","S7","U7","U8","" },
{ "W3","Y3","AA3","AA4","" },
{ "W5","Y5","AA5","AA6","" },
{ "W7","Y7","AA7","AA8","" },
{ "AC3","AE3","AG3","AG4","" },
{ "AC5","AE5","AG5","AG6","" },

{ "B10","F10","","","" },
{ "J10","N10","","","" },
{ "R10","V10","","","" },
{ "Z10","AD10","","","" },

{ "C15","P15","R15","T15","J15" },
{ "C16","P16","R16","T16","J16" },
{ "C17","P17","R17","T17","J17" },
{ "C18","P18","R18","T18","J18" },
{ "E19","P19","R19","T19","J19" },
{ "E20","P20","R20","T20","J20" },
{ "E21","P21","R21","T21","J21" },
{ "C22","P22","R22","T22","J22" },
{ "C23","P23","R23","T23","J23" },
{ "C24","P24","R24","T24","J24" },
{ "C25","P25","R25","T25","J25" },
{ "C26","P26","R26","T26","J26" },
{ "C27","P27","R27","T27","J27" },
{ "C28","P28","R28","T28","J28" },
{ "C29","P29","R29","T29","J29" },
{ "C30","P30","R30","T30","J30" },
{ "C31","P31","R31","T31","J31" },
{ "C32","P32","R32","T32","J32" },
{ "E33","P33","R33","T33","J33" },
{ "E34","P34","R34","T34","J34" },
{ "E35","P35","R35","T35","J35" },
{ "E36","P36","R36","T36","J36" },
{ "E37","P37","R37","T37","J37" },
{ "E38","P38","R38","T38","J38" },
{ "C39","P39","R39","T39","J39" },
{ "C40","P40","R40","T40","J40" },
{ "C41","P41","R41","T41","J41" },
{ "C42","P42","R42","T42","J42" },
{ "E43","P43","R43","T43","J43" },
{ "E44","P44","R44","T44","J44" },
{ "E45","P45","R45","T45","J45" },
{ "E46","P46","R46","T46","J56" },

{ "W15","AJ15","AL15","AN15","AD15" },
{ "W16","AJ16","AL16","AN16","AD16" },
{ "W17","AJ17","AL17","AN17","AD17" },
{ "W18","AJ18","AL18","AN18","AD18" },
{ "W19","AJ19","AL19","AN19","AD19" },
{ "W20","AJ20","AL20","AN20","AD20" },
{ "W21","AJ21","AL21","AN21","AD21" },
{ "W22","AJ22","AL22","AN22","AD22" },
{ "W23","AJ23","AL23","AN23","AD23" },
{ "W24","AJ24","AL24","AN24","AD24" },
{ "W25","AJ25","AL25","AN25","AD25" },
{ "Y26","AJ26","AL26","AN26","AD26" },
{ "W27","AJ27","AL27","AN27","AD27" },
{ "W28","AJ28","AL28","AN28","AD28" },
{ "W29","AJ29","AL29","AN29","AD29" },
{ "Y30","AJ30","AL30","AN30","AD30" },
{ "Y31","AJ31","AL31","AN31","AD31" },
{ "Y32","AJ32","AL32","AN32","AD32" },
{ "W33","AJ33","AL33","AN33","AD33" },
{ "W34","AJ34","AL34","AN34","AD34" },
{ "W35","AJ35","AL35","AN35","AD35" },
{ "Y36","AJ36","AL36","AN36","AD36" },
{ "W37","AJ37","AL37","AN37","AD37" },
{ "W38","AJ38","AL38","AN38","AD38" },
{ "W39","AJ39","AL39","AN39","AD39" },
{ "Y40","AJ40","AL40","AN40","AD40" },
{ "W41","AJ41","AL41","AN41","AD41" },
{ "W42","AJ42","AL42","AN42","AD42" },
{ "W43","AJ43","AL43","AN43","AD43" },
{ "W44","AJ44","AL44","AN44","AD44" },
{ "W45","AJ45","AL45","AN45","AD45" },
{ "W46","AJ46","AL15","AN15","AD15" }
};

typedef struct _PropertyNode
{
	int flag;
	char* n_r;
	wchar_t name[32];
	char* v_r;
	wchar_t value[6];
	char* h_r;
	wchar_t hard[6];
	char* d_r;
	wchar_t difficult[6];
	char* g_r;
	wchar_t grow[6];
}PropertyNode;


MainWindow::MainWindow(QWidget *parent) :
	QMainWindow(parent),
	ui(new Ui::MainWindow)
{
	ui->setupUi(this);

	this->setWindowTitle(QString::fromWCharArray(L"骰娘模板格式生成工具"));
	this->setWindowIcon(QIcon(":/new/prefix1/icon.ico"));

	ui->textEdit->setReadOnly(!FALSE);
	ui->textEdit_2->setReadOnly(!FALSE);
	ui->textEdit_2->setVisible(FALSE);
	ui->pushButton_export->setEnabled(FALSE);
	ui->pushButton_import->setEnabled(FALSE);
	ui->checkBox->setEnabled(FALSE);
	ui->checkBox_2->setEnabled(FALSE);
	

	if (drag_file_path) ui->lineEdit->setText(QString::fromLocal8Bit(drag_file_path));
}

MainWindow::~MainWindow()
{
	delete ui;
}

void EnableWidget(Ui::MainWindow *ui, int flag)
{
	ui->lineEdit->setEnabled(flag);
	ui->pushButton->setEnabled(flag);
	ui->pushButton_export->setEnabled(flag);
	ui->textEdit->setEnabled(flag);
#if ENABLE_EXPORT_SEPARATELY
	ui->checkBox_2->setEnabled(flag);
	ui->textEdit_2->setEnabled(flag);
#endif
#if ENABLE_IMPORT
	ui->checkBox->setEnabled(flag);
	ui->pushButton_import->setEnabled(flag);
#endif
}

void MainWindow::on_pushButton_clicked()
{
	QString fileName = QFileDialog::getOpenFileName(this, QString::fromWCharArray(L"打开人物卡")
		, NULL, QString::fromWCharArray(L"人物表格 (*.xlsx)"));
	ui->lineEdit->setText(fileName);
}

void MainWindow::on_lineEdit_textChanged(const QString &arg1)
{
	wchar_t* file_path = Utf8ToWchar(arg1.toUtf8().data());
	int flag = FALSE;

	wchar_t* c = wcsrchr(file_path, L'.');
	if (c && !wcsicmp(c, L".xlsx"))
		flag = 1;

	if (flag)
		flag = PathFileExists(file_path);
	free(file_path);

	if (flag)
	{
		ui->textEdit->clear();
		ui->textEdit_2->clear();
	}
	ui->textEdit->setReadOnly(!flag);
	ui->pushButton_export->setEnabled(flag);
#if ENABLE_EXPORT_SEPARATELY
	ui->checkBox_2->setEnabled(flag);
	ui->textEdit_2->setReadOnly(!flag);
#endif
#if ENABLE_IMPORT
	ui->pushButton_import->setEnabled(flag);
	ui->checkBox->setEnabled(flag);
#endif
}

void MainWindow::on_pushButton_export_clicked()
{
	EnableWidget(ui, FALSE);
	wchar_t* file_path = Utf8ToWchar(ui->lineEdit->text().toUtf8().data());
	ui->textEdit->clear();
	ui->statusBar->setStyleSheet("color:black");
	GetPropertyFromXlsx(file_path, ui);
	free(file_path);
	EnableWidget(ui, TRUE);
}

void MainWindow::on_pushButton_import_clicked()
{
	EnableWidget(ui, FALSE);
	wchar_t* file_path = Utf8ToWchar(ui->lineEdit->text().toUtf8().data());
	wchar_t* templet_string = Utf8ToWchar(ui->textEdit->toPlainText().toUtf8().data());
	ui->textEdit->clear();
	ui->statusBar->setStyleSheet("color:black");
	ui->checkBox_2->setChecked(FALSE);
	SetPropertyToXlsx(file_path, templet_string, ui);
	free(templet_string);
	free(file_path);
	EnableWidget(ui, TRUE);
}


void MainWindow::dragEnterEvent(QDragEnterEvent *e)
{
	if (e->mimeData()->hasFormat("text/uri-list"))
		e->acceptProposedAction();
}

void MainWindow::dropEvent(QDropEvent *e)
{
	QList<QUrl> urls = e->mimeData()->urls();
	if (urls.isEmpty())
		return;

	QString fileName = urls.first().toLocalFile();

	if (!fileName.isEmpty())
		ui->lineEdit->setText(fileName);
}



int SetPropertyToXlsx(wchar_t* file_path, wchar_t* templet_string, Ui::MainWindow *ui)
{

	wchar_t info_string[4096];

	wchar_t* unit_pointer = templet_string;
	PropertyNode import_list[128] = {};

	while (unit_pointer != (wchar_t*)2)
	{
		wchar_t* unit_separater = wcschr(unit_pointer, '|');
		if (unit_separater) *unit_separater = 0;
		unit_separater++;

		wchar_t* unit_value_string = wcschr(unit_pointer, ':');
		if (!unit_value_string) unit_value_string = wcschr(unit_pointer, '：');
		if (!unit_value_string) { unit_pointer = unit_separater; continue; }

		*unit_value_string = 0;
		unit_value_string++;

		wchar_t* unit_name_string = unit_pointer;
		while (wchar_t* p = wcschr(unit_name_string, ' ')) wcscpy_s(p, wcslen(p) + 1, p + 1);
		while (wchar_t* p = wcschr(unit_value_string, ' ')) wcscpy_s(p, wcslen(p) + 1, p + 1);

		if (wcslen(unit_name_string) >= 31) { unit_pointer = unit_separater; continue; }
		if (wcslen(unit_value_string) >= 31) { unit_pointer = unit_separater; continue; }

		for (int i = 0; i < 128; i++)
		{
			if (!import_list[i].name[0])
			{
				wcscpy_s(import_list[i].name, unit_name_string);
				wcscpy_s(import_list[i].value, unit_value_string);
				break;
			}
		}

		unit_pointer = unit_separater;
	}

	swprintf_s(info_string, L"正在解析 %s", file_path);
	ui->statusBar->showMessage(QString::fromWCharArray(info_string));

	HANDLE handle_file = CreateFile(file_path, GENERIC_READ,
		FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);

	if (handle_file == INVALID_HANDLE_VALUE)
	{
		ui->statusBar->setStyleSheet("color:red");
		swprintf_s(info_string, L"无法打开 \"%s\" : %s", file_path, GetLastError() ? GetErrorTips() : L"未知原因.");
		ui->statusBar->showMessage(QString::fromWCharArray(info_string));
		return 1;
	}

	LARGE_INTEGER file_size = {};
	GetFileSizeEx(handle_file, &file_size);

	if (file_size.QuadPart <= 0)
	{
		ui->statusBar->setStyleSheet("color:red");
		swprintf_s(info_string, L"\"%s\" 是空的", file_path);
		ui->statusBar->showMessage(QString::fromWCharArray(info_string));
		CloseHandle(handle_file);
		return 1;
	}

	void* zip_buf = malloc((size_t)file_size.QuadPart);

	DWORD read_size = 0;
	if (!ReadFile(handle_file, zip_buf, (DWORD)file_size.QuadPart, &read_size, NULL) || read_size != file_size.QuadPart)
	{
		ui->statusBar->setStyleSheet("color:red");
		swprintf_s(info_string, L"无法读取 \"%s\" : %s", file_path, GetLastError() ? GetErrorTips() : L"未知原因.");
		ui->statusBar->showMessage(QString::fromWCharArray(info_string));
		free(zip_buf);
		CloseHandle(handle_file);
		return 1;
	}

	CloseHandle(handle_file);

	HZIP old_hz = OpenZip(zip_buf, read_size, 0);
	ZIPENTRY ze;
	int index;

	swprintf_s(info_string, L"正在检查文件正确性");
	ui->statusBar->showMessage(QString::fromWCharArray(info_string));

	FindZipItem(old_hz, L"xl/worksheets/sheet1.xml", true, &index, &ze);
	if (index == -1)
	{
		ui->statusBar->setStyleSheet("color:red");
		swprintf_s(info_string, L"xlxs解析错误,没有找到 xl/worksheets/sheet1.xml");
		ui->statusBar->showMessage(QString::fromWCharArray(info_string));
		CloseZip(old_hz);
		free(zip_buf);
		return 1;
	}
	char *sheet_xml_buf = (char*)malloc(ze.unc_size + 8);
	memset(sheet_xml_buf, 0, ze.unc_size + 8);
	UnzipItem(old_hz, index, sheet_xml_buf, ze.unc_size);

	FindZipItem(old_hz, L"xl/sharedStrings.xml", true, &index, &ze);
	if (index == -1)
	{
		ui->statusBar->setStyleSheet("color:red");
		swprintf_s(info_string, L"xlxs解析错误,没有找到 xl/sharedStrings.xml");
		ui->statusBar->showMessage(QString::fromWCharArray(info_string));
		CloseZip(old_hz);
		free(sheet_xml_buf);
		free(zip_buf);
		return 1;
	}
	char *sharedStrings_xml_buf = (char*)malloc(ze.unc_size + 8);
	memset(sharedStrings_xml_buf, 0, ze.unc_size + 8);
	UnzipItem(old_hz, index, sharedStrings_xml_buf, ze.unc_size);


	PropertyNode info[76] = {};

	xml_document<> sheet_doc;
	xml_node<> *worksheet_node = NULL;
	xml_node<> *sheetData_node = NULL;
	xml_document<> sharedStrings_doc;
	xml_node<> *sst_node = NULL;
	HZIP new_hz = {};

	sheet_doc.parse<0>(sheet_xml_buf);
	sharedStrings_doc.parse<0>(sharedStrings_xml_buf);

	swprintf_s(info_string, L"正在检查XML正确性");
	ui->statusBar->showMessage(QString::fromWCharArray(info_string));

	worksheet_node = sheet_doc.first_node("worksheet");
	if (!worksheet_node)
	{
		ui->statusBar->setStyleSheet("color:red");
		swprintf_s(info_string, L"xml解析错误,没有找到 sheet1->worksheet");
		ui->statusBar->showMessage(QString::fromWCharArray(info_string));
		free(sheet_xml_buf); free(sharedStrings_xml_buf); CloseZip(old_hz); free(zip_buf); return 1;
	}

	sheetData_node = worksheet_node->first_node("sheetData");
	if (!sheetData_node)
	{
		ui->statusBar->setStyleSheet("color:red");
		swprintf_s(info_string, L"xml解析错误,没有找到 sheet1->worksheet->sheetData");
		ui->statusBar->showMessage(QString::fromWCharArray(info_string));
		free(sheet_xml_buf); free(sharedStrings_xml_buf); CloseZip(old_hz); free(zip_buf); return 1;
	}

	if (!sheetData_node->first_node("row"))
	{
		ui->statusBar->setStyleSheet("color:red");
		swprintf_s(info_string, L"xml解析错误,没有找到  sheet1->worksheet->sheetData->row");
		ui->statusBar->showMessage(QString::fromWCharArray(info_string));
		free(sheet_xml_buf); free(sharedStrings_xml_buf); CloseZip(old_hz); free(zip_buf); return 1;
	}

	sst_node = sharedStrings_doc.first_node("sst");
	if (!sheetData_node)
	{
		ui->statusBar->setStyleSheet("color:red");
		swprintf_s(info_string, L"xml解析错误,没有找到 sharedStrings->sst");
		ui->statusBar->showMessage(QString::fromWCharArray(info_string));
		free(sheet_xml_buf); free(sharedStrings_xml_buf); CloseZip(old_hz); free(zip_buf); return 1;
	}

	if (!sst_node->first_node("si"))
	{
		ui->statusBar->setStyleSheet("color:red");
		swprintf_s(info_string, L"xml解析错误,没有找到 sharedStrings->si");
		ui->statusBar->showMessage(QString::fromWCharArray(info_string));
		free(sheet_xml_buf); free(sharedStrings_xml_buf); CloseZip(old_hz); free(zip_buf); return 1;
	}


	swprintf_s(info_string, L"正在识别表格");
	ui->statusBar->showMessage(QString::fromWCharArray(info_string));

	int use_list_1 = 0;

	for (xml_node<> * row_node = sheetData_node->first_node("row"); row_node; row_node = row_node->next_sibling())
	{
		for (xml_node<> * c_node = row_node->first_node("c"); c_node; c_node = c_node->next_sibling())
		{
			xml_attribute<> * r_attr = c_node->first_attribute("r");


			if (!strcmp("O59", r_attr->value()))
			{

				xml_node<> * v_node = c_node->first_node("v");
				if (!v_node || !v_node->value())
					continue;

				r_attr = c_node->first_attribute("t");

				if (r_attr && r_attr->value()[0] == 's')
				{

					int si_index = 0;
					sscanf_s(v_node->value(), "%d", &si_index);

					xml_node<> *si_node = sst_node->first_node("si");

					for (int i = 0; i < si_index && si_node; i++)
						si_node = si_node->next_sibling();


					if (si_node)
					{
						xml_node<> *r_node = si_node->first_node("r");

						for (xml_node<> *t_node = r_node ? r_node->first_node("t") : si_node->first_node("t");
							t_node; t_node = t_node->next_sibling())
						{
							wchar_t* value_string = Utf8ToWchar(t_node->value());
							if (!wcscmp(L"货币:", value_string))
								use_list_1 = 1;

							free(value_string);
						}
					}

				}
				else
				{

					wchar_t* value_string = Utf8ToWchar(v_node->value());
					if (!wcscmp(L"货币:", value_string))
						use_list_1 = 1;

					free(value_string);
				}

				continue;
			}
		}
	}

	if (use_list_1)
	{
		for (int i = 0; i < 72; i++)
		{
			info[i].n_r = property_list_1[i][0];
			info[i].v_r = property_list_1[i][1];
			info[i].h_r = property_list_1[i][2];
			info[i].d_r = property_list_1[i][3];
			info[i].g_r = property_list_1[i][4];
		}
	}
	else
	{
		for (int i = 0; i < 72; i++)
		{
			info[i].n_r = property_list_2[i][0];
			info[i].v_r = property_list_2[i][1];
			info[i].h_r = property_list_2[i][2];
			info[i].d_r = property_list_2[i][3];
			info[i].g_r = property_list_2[i][4];
		}
	}

	swprintf_s(info_string, L"正在解析XML");
	ui->statusBar->showMessage(QString::fromWCharArray(info_string));

	for (xml_node<> * row_node = sheetData_node->first_node("row"); row_node; row_node = row_node->next_sibling())
	{
		for (xml_node<> * c_node = row_node->first_node("c"); c_node; c_node = c_node->next_sibling())
		{
			xml_attribute<> * r_attr = c_node->first_attribute("r");
			for (int i = 0; i < 72; i++)
			{
				if (!strcmp(info[i].n_r, r_attr->value()))
				{

					xml_node<> * v_node = c_node->first_node("v");
					if (!v_node || !v_node->value())
						break;

					r_attr = c_node->first_attribute("t");


					if (r_attr && r_attr->value()[0] == 's')
					{

						int si_index = 0;
						sscanf_s(v_node->value(), "%d", &si_index);

						xml_node<> *si_node = sst_node->first_node("si");

						for (int i = 0; i < si_index && si_node; i++)
							si_node = si_node->next_sibling();


						if (si_node)
						{
							xml_node<> *r_node = si_node->first_node("r");

							for (xml_node<> *t_node = r_node ? r_node->first_node("t") : si_node->first_node("t");
								t_node; t_node = t_node->next_sibling())
							{
								wchar_t* value_string = Utf8ToWchar(t_node->value());

								while (wchar_t* p = wcschr(value_string, L'\r')) wcscpy_s(p, wcslen(p) + 1, p + 1);
								while (wchar_t* p = wcschr(value_string, L'\n')) wcscpy_s(p, wcslen(p) + 1, p + 1);
								while (wchar_t* p = wcschr(value_string, L' ')) wcscpy_s(p, wcslen(p) + 1, p + 1);
								while (wchar_t* p = wcschr(value_string, L'Ω')) wcscpy_s(p, wcslen(p) + 1, p + 1);
								for (wchar_t* p = value_string; *p; p++)
								{
									if ((*p >= L'a' && *p <= L'z') || (*p >= L'A' && *p <= L'z'))
									{
										wcscpy_s(p, wcslen(p) + 1, p + 1);
										p--;
									}
								}

								wcscat_s(info[i].name, value_string);

								free(value_string);
							}
						}

					}
					else
					{

						wchar_t* value_string = Utf8ToWchar(v_node->value());

						while (wchar_t* p = wcschr(value_string, L'\r')) wcscpy_s(p, wcslen(p) + 1, p + 1);
						while (wchar_t* p = wcschr(value_string, L'\n')) wcscpy_s(p, wcslen(p) + 1, p + 1);
						while (wchar_t* p = wcschr(value_string, L' ')) wcscpy_s(p, wcslen(p) + 1, p + 1);
						while (wchar_t* p = wcschr(value_string, L'Ω')) wcscpy_s(p, wcslen(p) + 1, p + 1);
						for (wchar_t* p = value_string; *p; p++)
						{
							if ((*p >= L'a' && *p <= L'z') || (*p >= L'A' && *p <= L'z'))
							{
								wcscpy_s(p, wcslen(p) + 1, p + 1);
								p--;
							}
						}

						wcscpy_s(info[i].name, value_string);

						free(value_string);
					}

					for (int j = 0; j < 128; j++)
					{
						if (!wcscmp(info[i].name, import_list[j].name))
						{
							wcscpy_s(info[i].value, import_list[j].value);
							int new_value = 0;
							swscanf_s(import_list[j].value, L"%d", &new_value);

							swprintf_s(info[i].hard, L"%d", new_value / 2);
							swprintf_s(info[i].difficult, L"%d", new_value / 5);

							import_list[j].flag = 1;
							break;
						}
					}

					break;
				}
			}
		}
	}

	for (xml_node<> * row_node = sheetData_node->first_node("row"); row_node; row_node = row_node->next_sibling())
	{
		for (xml_node<> * c_node = row_node->first_node("c"); c_node; c_node = c_node->next_sibling())
		{
			xml_attribute<> * r_attr = c_node->first_attribute("r");
			for (int i = 0; i < 72; i++)
			{
				if (!strcmp(info[i].v_r, r_attr->value()))
				{
					r_attr = c_node->first_attribute("t");
					xml_node<> * v_node = c_node->first_node("v");

					if (v_node && v_node->value())
					{
						for (int j = 0; j < 128; j++)
						{
							if (!wcscmp(info[i].name, import_list[j].name))
							{
								wchar_t* value_string = Utf8ToWchar(v_node->value());

								wchar_t* decimal_point = wcschr(value_string, L'.');
								if (decimal_point)
									*decimal_point = 0;

								int old_value = 0;
								int new_value = 0;

								swscanf_s(value_string, L"%d", &old_value);
								swscanf_s(import_list[j].value, L"%d", &new_value);

								if (new_value != old_value)
									swprintf_s(info[i].grow, L"%d", new_value - old_value);

								free(value_string);
								break;
							}
						}

					}
					break;
				}
			}
		}
	}

	for (xml_node<> * row_node = sheetData_node->first_node("row"); row_node; row_node = row_node->next_sibling())
	{
		for (xml_node<> * c_node = row_node->first_node("c"); c_node; c_node = c_node->next_sibling())
		{
			xml_attribute<> * r_attr = c_node->first_attribute("r");
			for (int i = 0; i < 72; i++)
			{
				if (!strcmp(info[i].g_r, r_attr->value()) && info[i].grow[0])
				{
					r_attr = c_node->first_attribute("t");
					xml_node<> * v_node = c_node->first_node("v");

					if (v_node && v_node->value())
					{
						wchar_t* value_string = Utf8ToWchar(v_node->value());

						wchar_t* decimal_point = wcschr(value_string, L'.');
						if (decimal_point)
							*decimal_point = 0;

						int old_value = 0;
						int new_value = 0;

						swscanf_s(value_string, L"%d", &old_value);
						swscanf_s(info[i].grow, L"%d", &new_value);

						swprintf_s(info[i].grow, L"%d", new_value + old_value);

						free(value_string);
						break;
					}
					break;
				}
			}
		}
	}

	swprintf_s(info_string, L"正在导入数据");
	ui->statusBar->showMessage(QString::fromWCharArray(info_string));

	for (xml_node<> * row_node = sheetData_node->first_node("row"); row_node; row_node = row_node->next_sibling())
	{
		for (xml_node<> * c_node = row_node->first_node("c"); c_node; c_node = c_node->next_sibling())
		{
			xml_attribute<> * r_attr = c_node->first_attribute("r");
			for (int i = 0; i < 72; i++)
			{
				if (!strcmp(info[i].v_r, r_attr->value()) && info[i].value[0])
				{
					xml_node<> * v_node = c_node->first_node("v");
					char* value_string = WcharToUtf8(info[i].value);
					xml_node<> *new_node = sheet_doc.allocate_node(node_element, sheet_doc.allocate_string("v")
						, sheet_doc.allocate_string(value_string));

					c_node->insert_node(0, new_node);
					if (v_node) c_node->remove_node(v_node);

					free(value_string);
					break;
				}
				else if (!strcmp(info[i].h_r, r_attr->value()) && info[i].value[0])
				{
					xml_node<> * v_node = c_node->first_node("v");
					char* value_string = WcharToUtf8(info[i].hard);
					xml_node<> *new_node = sheet_doc.allocate_node(node_element, sheet_doc.allocate_string("v")
						, sheet_doc.allocate_string(value_string));

					c_node->insert_node(0, new_node);
					if (v_node) c_node->remove_node(v_node);

					free(value_string);
					break;
				}
				else if (!strcmp(info[i].d_r, r_attr->value()) && info[i].value[0])
				{
					xml_node<> * v_node = c_node->first_node("v");
					char* value_string = WcharToUtf8(info[i].difficult);
					xml_node<> *new_node = sheet_doc.allocate_node(node_element, sheet_doc.allocate_string("v")
						, sheet_doc.allocate_string(value_string));

					c_node->insert_node(0, new_node);
					if (v_node) c_node->remove_node(v_node);

					free(value_string);
					break;
				}
				else if (!strcmp(info[i].g_r, r_attr->value()) && info[i].value[0] && info[i].grow[0])
				{
					xml_node<> * v_node = c_node->first_node("v");
					char* value_string = WcharToUtf8(info[i].grow);
					xml_node<> *new_node = sheet_doc.allocate_node(node_element, sheet_doc.allocate_string("v")
						, sheet_doc.allocate_string(value_string));

					c_node->insert_node(0, new_node);
					if (v_node) c_node->remove_node(v_node);

					free(value_string);
					break;
				}
			}
		}
	}


	swprintf_s(info_string, L"正在写入新的表格");
	ui->statusBar->showMessage(QString::fromWCharArray(info_string));

	unsigned long new_zip_size = read_size * 2;
	new_hz = CreateZip(0, new_zip_size, 0);


	char* buffer = (char*)malloc(1024 * 1024);      // You are responsible for making the buffer large enough!
	char *end = print(buffer, sheet_doc, 0);      // end contains pointer to character after last printed character
	*end = 0;									  // Add string terminator after XML
	ZipAdd(new_hz, L"xl/worksheets/sheet1.xml", buffer, strlen(buffer));
	free(buffer);

	ze = {};
	GetZipItem(old_hz, -1, &ze);
	int numitems = ze.index;
	for (int zi = 0; zi < numitems; zi++)
	{
		ZIPENTRY ze;
		GetZipItem(old_hz, zi, &ze);	// fetch individual details

		if (!wcscmp(ze.name, L"xl/worksheets/sheet1.xml"))
			continue;

		char *ibuf = (char*)malloc(ze.unc_size);
		UnzipItem(old_hz, zi, ibuf, ze.unc_size);	// e.g. the item's name.
		ZipAdd(new_hz, ze.name, ibuf, ze.unc_size);
		free(ibuf);
	}

	ZipGetMemory(new_hz, (void**)&buffer, &new_zip_size);

	if (ui->checkBox->isChecked())
	{
		wchar_t new_file_path[MAX_PATH] = {};
		wcscpy_s(new_file_path, file_path);
		wcscat_s(new_file_path, L".bak");
		if (!MoveFile(file_path, new_file_path))
		{
			ui->statusBar->setStyleSheet("color:red");
			if (PathFileExists(new_file_path))
				swprintf_s(info_string, L"无法移动 \"%s\" 至 \"%s\" : 存在备份文件", file_path, new_file_path);
			else
				swprintf_s(info_string, L"无法移动 \"%s\" 至 \"%s\" : %s", file_path, new_file_path, GetLastError() ? GetErrorTips() : L"未知原因.");
			ui->statusBar->showMessage(QString::fromWCharArray(info_string));
			free(sheet_xml_buf); free(sharedStrings_xml_buf); CloseZip(old_hz); return 1;
		}
	}
	else
	{
		if (!DeleteFile(file_path))
		{
			ui->statusBar->setStyleSheet("color:red");
			swprintf_s(info_string, L"无法删除 \"%s\" : %s", file_path, GetLastError() ? GetErrorTips() : L"未知原因.");
			ui->statusBar->showMessage(QString::fromWCharArray(info_string));
			free(sheet_xml_buf); free(sharedStrings_xml_buf); CloseZip(old_hz); return 1;
		}
	}

	handle_file = CreateFile(file_path, GENERIC_WRITE, 0, 0, CREATE_NEW, FILE_ATTRIBUTE_NORMAL, 0);
	if (handle_file == INVALID_HANDLE_VALUE)
	{
		ui->statusBar->setStyleSheet("color:red");
		swprintf_s(info_string, L"无法打开 \"%s\" : %s", file_path, GetLastError() ? GetErrorTips() : L"未知原因.");
		ui->statusBar->showMessage(QString::fromWCharArray(info_string));
		free(sheet_xml_buf);
		free(sharedStrings_xml_buf);
		CloseZip(old_hz);
		CloseZip(new_hz);
		return 1;
	}
	DWORD writ; WriteFile(handle_file, buffer, new_zip_size, &writ, 0);
	CloseHandle(handle_file);

	free(sheet_xml_buf);
	free(sharedStrings_xml_buf);

	CloseZip(old_hz);
	free(zip_buf);

	CloseZip(new_hz);

	for (int i = 0; i < 128; i++)
		if (!import_list[i].flag && import_list[i].name[0] &&
			wcscmp(import_list[i].name, L"初始生命") &&
			wcscmp(import_list[i].name, L"初始理智") &&
			wcscmp(import_list[i].name, L"初始幸运") &&
			wcscmp(import_list[i].name, L"初始魔法"))
		{
			swprintf_s(info_string, L"[%s]没有被导入,请确认表格中存在此技能", import_list[i].name);
			ui->textEdit->append(QString::fromWCharArray(info_string));
		}

	swprintf_s(info_string, L"导入完成");
	ui->statusBar->showMessage(QString::fromWCharArray(info_string));

	return 0;
}



























int GetPropertyFromXlsx(wchar_t* file_path, Ui::MainWindow *ui)
{
	ui->statusBar->setStyleSheet("color:black");
	name[0] = 0;

	wchar_t info_string[4096];

	swprintf_s(info_string, L"正在解析 %s", file_path);
	ui->statusBar->showMessage(QString::fromWCharArray(info_string));

	HANDLE handle_file = CreateFile(file_path, GENERIC_READ,
		FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);

	if (handle_file == INVALID_HANDLE_VALUE)
	{
		ui->statusBar->setStyleSheet("color:red");
		swprintf_s(info_string, L"无法打开 \"%s\" : %s", file_path, GetLastError() ? GetErrorTips() : L"未知原因.");
		ui->statusBar->showMessage(QString::fromWCharArray(info_string));
		return 1;
	}

	LARGE_INTEGER file_size = {};
	GetFileSizeEx(handle_file, &file_size);

	if (file_size.QuadPart <= 0)
	{
		ui->statusBar->setStyleSheet("color:red");
		swprintf_s(info_string, L"\"%s\" 是空的", file_path);
		ui->statusBar->showMessage(QString::fromWCharArray(info_string));
		CloseHandle(handle_file);
		return 1;
	}

	void* zip_buf = malloc((size_t)file_size.QuadPart);

	DWORD read_size = 0;
	if (!ReadFile(handle_file, zip_buf, (DWORD)file_size.QuadPart, &read_size, NULL) || read_size != file_size.QuadPart)
	{
		ui->statusBar->setStyleSheet("color:red");
		swprintf_s(info_string, L"无法读取 \"%s\" : %s", file_path, GetLastError() ? GetErrorTips() : L"未知原因.");
		ui->statusBar->showMessage(QString::fromWCharArray(info_string));
		free(zip_buf);
		CloseHandle(handle_file);
		return 1;
	}

	CloseHandle(handle_file);

	HZIP hz = OpenZip(zip_buf, read_size, 0);
	ZIPENTRY ze;
	int index;


	swprintf_s(info_string, L"正在检查表格正确性");
	ui->statusBar->showMessage(QString::fromWCharArray(info_string));


	FindZipItem(hz, L"xl/worksheets/sheet1.xml", true, &index, &ze);
	if (index == -1)
	{
		ui->statusBar->setStyleSheet("color:red");
		swprintf_s(info_string, L"xlxs解析错误,没有找到 xl/worksheets/sheet1.xml");
		ui->statusBar->showMessage(QString::fromWCharArray(info_string));
		CloseZip(hz);
		free(zip_buf);
		return 1;
	}

	char *sheet_xml_buf = (char*)malloc(ze.unc_size + 8);
	memset(sheet_xml_buf, 0, ze.unc_size + 8);
	UnzipItem(hz, index, sheet_xml_buf, ze.unc_size);

	FindZipItem(hz, L"xl/sharedStrings.xml", true, &index, &ze);
	if (index == -1)
	{
		ui->statusBar->setStyleSheet("color:red");
		swprintf_s(info_string, L"xlxs解析错误,没有找到 xl/sharedStrings.xml");
		ui->statusBar->showMessage(QString::fromWCharArray(info_string));
		CloseZip(hz);
		free(sheet_xml_buf);
		free(zip_buf);
		return 1;
	}
	char *sharedStrings_xml_buf = (char*)malloc(ze.unc_size + 8);
	memset(sharedStrings_xml_buf, 0, ze.unc_size + 8);
	UnzipItem(hz, index, sharedStrings_xml_buf, ze.unc_size);

	CloseZip(hz);
	free(zip_buf);

	PropertyNode info[76] = {};

	xml_document<> sheet_doc;
	xml_node<> *worksheet_node = NULL;
	xml_node<> *sheetData_node = NULL;
	xml_document<> sharedStrings_doc;
	xml_node<> *sst_node = NULL;
	

	sheet_doc.parse<0>(sheet_xml_buf);
	sharedStrings_doc.parse<0>(sharedStrings_xml_buf);


	swprintf_s(info_string, L"正在检查XML正确性");
	ui->statusBar->showMessage(QString::fromWCharArray(info_string));

	worksheet_node = sheet_doc.first_node("worksheet");
	if (!worksheet_node)
	{
		ui->statusBar->setStyleSheet("color:red");
		swprintf_s(info_string, L"xml解析错误,没有找到 sheet1->worksheet");
		ui->statusBar->showMessage(QString::fromWCharArray(info_string));
		free(sheet_xml_buf); free(sharedStrings_xml_buf); return 1;
	}

	sheetData_node = worksheet_node->first_node("sheetData");
	if (!sheetData_node)
	{
		ui->statusBar->setStyleSheet("color:red");
		swprintf_s(info_string, L"xml解析错误,没有找到 sheet1->worksheet->sheetData");
		ui->statusBar->showMessage(QString::fromWCharArray(info_string));
		free(sheet_xml_buf); free(sharedStrings_xml_buf); return 1;
	}

	if (!sheetData_node->first_node("row")) {
		ui->statusBar->setStyleSheet("color:red");
		swprintf_s(info_string, L"xml解析错误,没有找到  sheet1->worksheet->sheetData->row");
		ui->statusBar->showMessage(QString::fromWCharArray(info_string));
		free(sheet_xml_buf); free(sharedStrings_xml_buf); return 1;
	}

	sst_node = sharedStrings_doc.first_node("sst");
	if (!sheetData_node)
	{
		ui->statusBar->setStyleSheet("color:red");
		swprintf_s(info_string, L"xml解析错误,没有找到 sharedStrings->sst");
		ui->statusBar->showMessage(QString::fromWCharArray(info_string));
		free(sheet_xml_buf); free(sharedStrings_xml_buf); return 1;
	}

	if (!sst_node->first_node("si"))
	{
		ui->statusBar->setStyleSheet("color:red");
		swprintf_s(info_string, L"xml解析错误,没有找到 sharedStrings->si");
		ui->statusBar->showMessage(QString::fromWCharArray(info_string));
		free(sheet_xml_buf); free(sharedStrings_xml_buf); return 1;
	}


	swprintf_s(info_string, L"正在识别表格");
	ui->statusBar->showMessage(QString::fromWCharArray(info_string));

	int use_list_1 = 0;

	for (xml_node<> * row_node = sheetData_node->first_node("row"); row_node; row_node = row_node->next_sibling())
	{
		for (xml_node<> * c_node = row_node->first_node("c"); c_node; c_node = c_node->next_sibling())
		{
			xml_attribute<> * r_attr = c_node->first_attribute("r");


			if (!strcmp("O59", r_attr->value()))
			{

				xml_node<> * v_node = c_node->first_node("v");
				if (!v_node || !v_node->value())
					continue;

				r_attr = c_node->first_attribute("t");

				if (r_attr && r_attr->value()[0] == 's')
				{

					int si_index = 0;
					sscanf_s(v_node->value(), "%d", &si_index);

					xml_node<> *si_node = sst_node->first_node("si");

					for (int i = 0; i < si_index && si_node; i++)
						si_node = si_node->next_sibling();


					if (si_node)
					{
						xml_node<> *r_node = si_node->first_node("r");

						for (xml_node<> *t_node = r_node ? r_node->first_node("t") : si_node->first_node("t");
							t_node; t_node = t_node->next_sibling())
						{
							wchar_t* value_string = Utf8ToWchar(t_node->value());

							if (!wcscmp(L"货币:", value_string))
								use_list_1 = 1;

							free(value_string);
						}
					}

				}
				else
				{

					wchar_t* value_string = Utf8ToWchar(v_node->value());

					if (!wcscmp(L"货币:", value_string))
						use_list_1 = 1;

					free(value_string);
				}

				continue;
			}
		}
	}

	if (use_list_1)
	{
		for (int i = 0; i < 72; i++)
		{
			info[i].n_r = property_list_1[i][0];
			info[i].v_r = property_list_1[i][1];
		}
	}
	else
	{
		for (int i = 0; i < 72; i++)
		{
			info[i].n_r = property_list_2[i][0];
			info[i].v_r = property_list_2[i][1];
		}
	}

	


	swprintf_s(info_string, L"正在解析XML");
	ui->statusBar->showMessage(QString::fromWCharArray(info_string));

	for (xml_node<> * row_node = sheetData_node->first_node("row"); row_node; row_node = row_node->next_sibling())
	{
		for (xml_node<> * c_node = row_node->first_node("c"); c_node; c_node = c_node->next_sibling())
		{
			xml_attribute<> * r_attr = c_node->first_attribute("r");


			if (!strcmp(name_r, r_attr->value()))
			{

				xml_node<> * v_node = c_node->first_node("v");
				if (!v_node || !v_node->value())
					continue;

				r_attr = c_node->first_attribute("t");

				if (r_attr && r_attr->value()[0] == 's')
				{

					int si_index = 0;
					sscanf_s(v_node->value(), "%d", &si_index);

					xml_node<> *si_node = sst_node->first_node("si");

					for (int i = 0; i < si_index && si_node; i++)
						si_node = si_node->next_sibling();


					if (si_node)
					{
						xml_node<> *r_node = si_node->first_node("r");

						for (xml_node<> *t_node = r_node ? r_node->first_node("t") : si_node->first_node("t");
							t_node; t_node = t_node->next_sibling())
						{
							wchar_t* value_string = Utf8ToWchar(t_node->value());

							while (wchar_t* p = wcschr(value_string, L'\r')) wcscpy_s(p, wcslen(p) + 1, p + 1);
							while (wchar_t* p = wcschr(value_string, L'\n')) wcscpy_s(p, wcslen(p) + 1, p + 1);
							while (wchar_t* p = wcschr(value_string, L' ')) wcscpy_s(p, wcslen(p) + 1, p + 1);
							while (wchar_t* p = wcschr(value_string, L'Ω')) wcscpy_s(p, wcslen(p) + 1, p + 1);
							for (wchar_t* p = value_string; *p; p++)
							{
								if ((*p >= L'a' && *p <= L'z') || (*p >= L'A' && *p <= L'z'))
								{
									wcscpy_s(p, wcslen(p) + 1, p + 1);
									p--;
								}
							}

							wcscat_s(name, value_string);

							free(value_string);
						}
					}

				}
				else
				{

					wchar_t* value_string = Utf8ToWchar(v_node->value());

					while (wchar_t* p = wcschr(value_string, L'\r')) wcscpy_s(p, wcslen(p) + 1, p + 1);
					while (wchar_t* p = wcschr(value_string, L'\n')) wcscpy_s(p, wcslen(p) + 1, p + 1);
					while (wchar_t* p = wcschr(value_string, L' ')) wcscpy_s(p, wcslen(p) + 1, p + 1);
					while (wchar_t* p = wcschr(value_string, L'Ω')) wcscpy_s(p, wcslen(p) + 1, p + 1);
					for (wchar_t* p = value_string; *p; p++)
					{
						if ((*p >= L'a' && *p <= L'z') || (*p >= L'A' && *p <= L'z'))
						{
							wcscpy_s(p, wcslen(p) + 1, p + 1);
							p--;
						}
					}

					wcscpy_s(name, value_string);

					free(value_string);
				}

				continue;
			}

			for (int i = 0; i < 72; i++)
			{
				if (!strcmp(info[i].n_r, r_attr->value()))
				{

					xml_node<> * v_node = c_node->first_node("v");
					if (!v_node || !v_node->value())
						break;

					r_attr = c_node->first_attribute("t");


					if (r_attr && r_attr->value()[0] == 's')
					{

						int si_index = 0;
						sscanf_s(v_node->value(), "%d", &si_index);

						xml_node<> *si_node = sst_node->first_node("si");

						for (int i = 0; i < si_index && si_node; i++)
							si_node = si_node->next_sibling();


						if (si_node)
						{
							xml_node<> *r_node = si_node->first_node("r");

							for (xml_node<> *t_node = r_node ? r_node->first_node("t") : si_node->first_node("t");
								t_node; t_node = t_node->next_sibling())
							{
								wchar_t* value_string = Utf8ToWchar(t_node->value());

								while (wchar_t* p = wcschr(value_string, L'\r')) wcscpy_s(p, wcslen(p) + 1, p + 1);
								while (wchar_t* p = wcschr(value_string, L'\n')) wcscpy_s(p, wcslen(p) + 1, p + 1);
								while (wchar_t* p = wcschr(value_string, L' ')) wcscpy_s(p, wcslen(p) + 1, p + 1);
								while (wchar_t* p = wcschr(value_string, L'Ω')) wcscpy_s(p, wcslen(p) + 1, p + 1);
								for (wchar_t* p = value_string; *p; p++)
								{
									if ((*p >= L'a' && *p <= L'z') || (*p >= L'A' && *p <= L'z'))
									{
										wcscpy_s(p, wcslen(p) + 1, p + 1);
										p--;
									}
								}

								wcscat_s(info[i].name, value_string);

								free(value_string);
							}
						}

					}
					else
					{

						wchar_t* value_string = Utf8ToWchar(v_node->value());

						while (wchar_t* p = wcschr(value_string, L'\r')) wcscpy_s(p, wcslen(p) + 1, p + 1);
						while (wchar_t* p = wcschr(value_string, L'\n')) wcscpy_s(p, wcslen(p) + 1, p + 1);
						while (wchar_t* p = wcschr(value_string, L' ')) wcscpy_s(p, wcslen(p) + 1, p + 1);
						while (wchar_t* p = wcschr(value_string, L'Ω')) wcscpy_s(p, wcslen(p) + 1, p + 1);
						for (wchar_t* p = value_string; *p; p++)
						{
							if ((*p >= L'a' && *p <= L'z') || (*p >= L'A' && *p <= L'z'))
							{
								wcscpy_s(p, wcslen(p) + 1, p + 1);
								p--;
							}
						}

						wcscpy_s(info[i].name, value_string);

						free(value_string);
					}

					break;
				}
				else if (!strcmp(info[i].v_r, r_attr->value()))
				{
					r_attr = c_node->first_attribute("t");
					xml_node<> * v_node = c_node->first_node("v");

					if (v_node && v_node->value())
					{
						wchar_t* value_string = Utf8ToWchar(v_node->value());

						wchar_t* decimal_point = wcschr(value_string, L'.');
						if (decimal_point)
							*decimal_point = 0;

						wcscpy_s(info[i].value, value_string);

						free(value_string);
					}
					break;
				}
			}


		}
	}

	free(sheet_xml_buf);
	free(sharedStrings_xml_buf);


	swprintf_s(info_string, L"正在导出数据");
	ui->statusBar->showMessage(QString::fromWCharArray(info_string));

	wcscpy_s(info[72].name, L"初始生命");
	for (int i = 0; i < 72; i++)
		if (!wcscmp(info[i].name, L"生命"))
			wcscpy_s(info[72].value, info[i].value);

	wcscpy_s(info[73].name, L"初始理智");
	for (int i = 0; i < 72; i++)
		if (!wcscmp(info[i].name, L"理智"))
			wcscpy_s(info[73].value, info[i].value);

	wcscpy_s(info[74].name, L"初始幸运");
	for (int i = 0; i < 72; i++)
		if (!wcscmp(info[i].name, L"幸运"))
			wcscpy_s(info[74].value, info[i].value);

	wcscpy_s(info[75].name, L"初始魔法");
	for (int i = 0; i < 72; i++)
		if (!wcscmp(info[i].name, L"魔法"))
			wcscpy_s(info[75].value, info[i].value);

	if (!ui->checkBox_2->isChecked())
	{
		int separater_flag = 0;
		for (int i = 0; i < 76; i++)
		{
			if (info[i].name[0] != 0 && info->value[0] != 0)
			{
				if (!separater_flag)
				{
					swprintf_s(info_string, L".st ");
					ui->textEdit->moveCursor(QTextCursor::End);
					ui->textEdit->insertPlainText(QString::fromWCharArray(info_string));
					separater_flag = 1;
				}
				else
				{
					swprintf_s(info_string, L"|");
					ui->textEdit->moveCursor(QTextCursor::End);
					ui->textEdit->insertPlainText(QString::fromWCharArray(info_string));
				}
				swprintf_s(info_string, L"%s:%s", info[i].name, info[i].value);
				ui->textEdit->moveCursor(QTextCursor::End);
				ui->textEdit->insertPlainText(QString::fromWCharArray(info_string));
			}
		}
	}
	else
	{
		int separater_flag = 0;
		for (int i = 0; i < 38; i++)
		{
			if (info[i].name[0] != 0 && info->value[0] != 0)
			{
				if (!separater_flag)
				{
					swprintf_s(info_string, L".st ");
					ui->textEdit->moveCursor(QTextCursor::End);
					ui->textEdit->insertPlainText(QString::fromWCharArray(info_string));
					separater_flag = 1;
				}
				else
				{
					swprintf_s(info_string, L"|");
					ui->textEdit->moveCursor(QTextCursor::End);
					ui->textEdit->insertPlainText(QString::fromWCharArray(info_string));
				}
				swprintf_s(info_string, L"%s:%s", info[i].name, info[i].value);
				ui->textEdit->moveCursor(QTextCursor::End);
				ui->textEdit->insertPlainText(QString::fromWCharArray(info_string));
			}
		}
		separater_flag = 0;
		for (int i = 38; i < 76; i++)
		{
			if (info[i].name[0] != 0 && info->value[0] != 0)
			{
				if (!separater_flag)
				{
					swprintf_s(info_string, L".st ");
					ui->textEdit_2->moveCursor(QTextCursor::End);
					ui->textEdit_2->insertPlainText(QString::fromWCharArray(info_string));
					separater_flag = 1;
				}
				else
				{
					swprintf_s(info_string, L"|");
					ui->textEdit_2->moveCursor(QTextCursor::End);
					ui->textEdit_2->insertPlainText(QString::fromWCharArray(info_string));
				}
				swprintf_s(info_string, L"%s:%s", info[i].name, info[i].value);
				ui->textEdit_2->moveCursor(QTextCursor::End);
				ui->textEdit_2->insertPlainText(QString::fromWCharArray(info_string));
			}
		}
	}

	swprintf_s(info_string, L"解析完成");
	ui->statusBar->showMessage(QString::fromWCharArray(info_string));

	return 0;

}


wchar_t* GetErrorTips()
{
	void* err_msg = NULL;
	static wchar_t ret_info[256];

	int langid = MAKELANGID(LANG_CHINESE, SUBLANG_CHINESE_SIMPLIFIED);

	if (!FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
		NULL, GetLastError(), langid, (wchar_t*)&err_msg, 0, NULL)) return NULL;
	wcscpy_s(ret_info, _countof(ret_info), (wchar_t*)err_msg);
	LocalFree(err_msg);

	while (wchar_t* p = wcschr(ret_info, L'\r'))wcscpy_s(p, wcslen(p) + 1, p + 1);
	while (wchar_t* p = wcschr(ret_info, L'\n')) wcscpy_s(p, wcslen(p) + 1, p + 1);
	while (wchar_t* p = wcschr(ret_info, L'.')) wcscpy_s(p, wcslen(p) + 1, p + 1);
	while (wchar_t* p = wcschr(ret_info, L':')) wcscpy_s(p, wcslen(p) + 1, p + 1);

	return ret_info;

}


wchar_t* Utf8ToWchar(char* utf8_str)
{
	int wchar_len = MultiByteToWideChar(CP_UTF8, NULL, utf8_str, -1, NULL, 0);
	wchar_t* unicode_str = (wchar_t*)malloc(sizeof(wchar_t)*wchar_len);
	MultiByteToWideChar(CP_UTF8, NULL, utf8_str, -1, unicode_str, wchar_len);;
	return unicode_str;
}

char* WcharToUtf8(wchar_t* unicode_str)
{
	int utf8_len = WideCharToMultiByte(CP_UTF8, NULL, unicode_str, -1, NULL, 0, NULL, NULL);
	char *utf8_str = (char*)malloc(sizeof(char)*utf8_len);
	WideCharToMultiByte(CP_UTF8, NULL, unicode_str, -1, utf8_str, utf8_len, NULL, NULL);
	return utf8_str;
}

void MainWindow::on_checkBox_2_clicked(bool checked)
{
	ui->textEdit_2->setVisible(checked);
	ui->textEdit->clear();
	ui->textEdit_2->clear();
	ui->pushButton_import->setEnabled(!checked);
}
