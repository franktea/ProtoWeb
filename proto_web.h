/*
 * proto_web.h
 *
 *  Created on: May 14, 2019
 *      Author: frank
 */

#ifndef PROTO_WEB_H_
#define PROTO_WEB_H_

#include <Wt/WApplication.h>
#include <Wt/WBreak.h>
#include <Wt/WContainerWidget.h>
#include <Wt/WLineEdit.h>
#include <Wt/WPushButton.h>
#include <Wt/WText.h>
#include <Wt/WTextArea.h>
#include <Wt/WCheckBox.h>
#include <Wt/WComboBox.h>
#include <Wt/WTable.h>
#include <Wt/WTableCell.h>
#include <Wt/WAbstractTableModel.h>
#include <iostream>
#include <fstream>
#include <algorithm>
#include <google/protobuf/descriptor.h>
#include <google/protobuf/descriptor.pb.h>
#include <google/protobuf/dynamic_message.h>
#include <google/protobuf/compiler/importer.h>
#include <string>
#include <boost/filesystem.hpp>
#include <map>

using namespace google::protobuf;
using namespace google::protobuf::compiler;

class AbstractProtoItem;
class MessageItem;
class TextItem;

class AbstractProtoItem : public Wt::WContainerWidget
{
public:
	static std::unique_ptr<AbstractProtoItem> Create(Importer& importer, const std::string& message_name, bool root = false);
	static std::unique_ptr<AbstractProtoItem> Create(Importer& importer, const FieldDescriptor* fd, bool check_repeated = true);

	virtual void ToProtoString(std::string& result) = 0; // to text_format string
protected:
	AbstractProtoItem(){}
};

class MessageItem : public AbstractProtoItem
{
public:
	MessageItem(Importer& importer, const std::string& message_name, bool root = false);

	void ToProtoString(std::string& result) override;
private:
	bool root_; // 是否根结点，根结点生成text format的时候不需要外部的大括号
	std::vector<AbstractProtoItem*> children_; // 子控件，用来生成text format
	Wt::WTable* table_; // 用table来排成两列，无table会被排成一列。
};

class TextItem : public AbstractProtoItem
{
public:
	TextItem(Importer& importer, const FieldDescriptor* fd) : fd_(fd)
	{
		input_box_ = this->addWidget(std::make_unique<Wt::WLineEdit>());
	}

	void ToProtoString(std::string& result) override;
private:
	const FieldDescriptor* fd_;
	Wt::WLineEdit* input_box_;
};

class NumberItem : public AbstractProtoItem
{
public:
	NumberItem(Importer& importer, const FieldDescriptor* fd) : fd_(fd)
	{
		input_box_ = this->addWidget(std::make_unique<Wt::WLineEdit>());
	}
	void ToProtoString(std::string& result) override;
private:
	const FieldDescriptor* fd_;
	Wt::WLineEdit* input_box_;
};

class EnumItem : public AbstractProtoItem
{
public:
	EnumItem(Importer& importer, const FieldDescriptor* fd) : fd_(fd)
	{
		combobox_ = this->addWidget(std::make_unique<Wt::WComboBox>());
		const EnumDescriptor* ed = fd->enum_type();
		for(int i = 0; i < ed->value_count(); ++i)
		{
			const EnumValueDescriptor* evd = ed->value(i);
			combobox_->addItem(evd->name());
		}
	}

	void ToProtoString(std::string& result) override;
private:
	const FieldDescriptor* fd_;
	Wt::WComboBox* combobox_;
};

class BooleanItem : public AbstractProtoItem
{
public:
	BooleanItem(Importer& importer, const FieldDescriptor* fd):fd_(fd)
	{
		checkbox_ = this->addWidget(std::make_unique<Wt::WCheckBox>());
	}

	void ToProtoString(std::string& result) override;
private:
	const FieldDescriptor* fd_;
	Wt::WCheckBox* checkbox_;
};

class ReaptedItem : public AbstractProtoItem
{
public:
	ReaptedItem(Importer& importer, const FieldDescriptor* fd);
	void ToProtoString(std::string& result) override;
private:
	std::vector<AbstractProtoItem*> items_;
	std::vector<Wt::WPushButton*> delete_buttons_; // 存放所有的删除按钮，删除的时候在此处查找index
	Wt::WPushButton* add_button_;
	Wt::WTable* table_; // 用table组织，第一列显示一列减号按钮，用来删除当前项
	Importer& importer_;
	const FieldDescriptor* fd_;
};

#endif /* PROTO_WEB_H_ */
