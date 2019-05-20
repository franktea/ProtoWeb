/*
 * visual_proto.cpp
 *
 *  Created on: May 14, 2019
 *      Author: frank
 */
#include "proto_web.h"

std::unique_ptr<AbstractProtoItem> AbstractProtoItem::Create(Importer& importer,
		const std::string& message_name, bool root)
{
	auto ptr = new MessageItem(importer, message_name, root);
	return std::unique_ptr<AbstractProtoItem>(ptr);
}

std::unique_ptr<AbstractProtoItem> AbstractProtoItem::Create(Importer& importer,
		const FieldDescriptor* fd, bool check_repeated)
{
	if(check_repeated && fd->is_repeated())
	{
		auto ptr = new ReaptedItem(importer, fd);
		return std::unique_ptr<AbstractProtoItem>(ptr);
	}

	switch(fd->type())
	{
	case FieldDescriptor::TYPE_MESSAGE: //        = 11,  // Length-delimited message.
	{
		std::string type_name = fd->message_type()->full_name();
		return AbstractProtoItem::Create(importer, type_name);
	}
		break;
	case FieldDescriptor::TYPE_BOOL: //           = 8,   // bool, varint on the wire.
	{
		auto ptr = new BooleanItem(importer, fd);
		return std::unique_ptr<AbstractProtoItem>(ptr);
	}
		break;
	case FieldDescriptor::TYPE_ENUM: //           = 14,  // Enum, varint on the wire
	{
		auto ptr = new EnumItem(importer, fd);
		return std::unique_ptr<AbstractProtoItem>(ptr);
	}
		break;
	case FieldDescriptor::TYPE_STRING: //         = 9,   // UTF-8 text.
	{
		auto ptr = new TextItem(importer, fd);
		return std::unique_ptr<AbstractProtoItem>(ptr);
	}
		break;
	case FieldDescriptor::TYPE_DOUBLE: //         = 1,   // double, exactly eight bytes on the wire.
	case FieldDescriptor::TYPE_FLOAT: //          = 2,   // float, exactly four bytes on the wire.
	case FieldDescriptor::TYPE_INT64: //          = 3,   // int64, varint on the wire.  Negative numbers
	case FieldDescriptor::TYPE_UINT64: //         = 4,   // uint64, varint on the wire.
	case FieldDescriptor::TYPE_INT32: //          = 5,   // int32, varint on the wire.  Negative numbers
	case FieldDescriptor::TYPE_FIXED64: //        = 6,   // uint64, exactly eight bytes on the wire.
	case FieldDescriptor::TYPE_FIXED32: //        = 7,   // uint32, exactly four bytes on the wire.
	case FieldDescriptor::TYPE_GROUP: //          = 10,  // Tag-delimited message.  Deprecated.
	case FieldDescriptor::TYPE_UINT32: //         = 13,  // uint32, varint on the wire
	case FieldDescriptor::TYPE_SFIXED32: //       = 15,  // int32, exactly four bytes on the wire
	case FieldDescriptor::TYPE_SFIXED64: //       = 16,  // int64, exactly eight bytes on the wire
	case FieldDescriptor::TYPE_SINT32: //         = 17,  // int32, ZigZag-encoded varint on the wire
	case FieldDescriptor::TYPE_SINT64: //         = 18,  // int64, ZigZag-encoded varint on the wire
	{
		auto ptr = new NumberItem(importer, fd);
		return std::unique_ptr<AbstractProtoItem>(ptr);
	}
	break;
	case FieldDescriptor::TYPE_BYTES: //          = 12,  // Arbitrary byte array.
		//TODO:
		break;
	}

	return std::unique_ptr<AbstractProtoItem>();
}

MessageItem::MessageItem(Importer& importer, const std::string& message_name,
		bool root) : root_(root)
{
	const Descriptor * descriptor = importer.pool()->FindMessageTypeByName(message_name);
	if(nullptr == descriptor)
	{
		this->addWidget(std::make_unique<Wt::WText>(Wt::WString("error: can not find message {1}").arg(message_name)));
		return;
	}

	table_ = this->addWidget(std::make_unique<Wt::WTable>());

	for(int i = 0; i < descriptor->field_count(); ++i)
	{
		const FieldDescriptor* fd = descriptor->field(i);
		table_->elementAt(i, 0)->addWidget(std::make_unique<Wt::WText>(fd->name()));
		auto item = Create(importer, fd);
		children_.push_back(item.get());
		table_->elementAt(i, 1)->addWidget(std::move(item));
	}
}

void MessageItem::ToProtoString(std::string& result)
{
	if(!root_)
		result.append("{");

	for(AbstractProtoItem* item : children_)
	{
		item->ToProtoString(result);
	}

	if(!root_)
		result.append("}");
}

void TextItem::ToProtoString(std::string& result)
{
	result.append(" ");
	result.append(fd_->name());
	result.append(": \"");
	result.append(input_box_->text().toUTF8());
	result.append("\"");
}

void EnumItem::ToProtoString(std::string& result)
{
	result.append(" ");
	result.append(fd_->name());
	result.append(": ");
	result.append(std::to_string(combobox_->currentIndex()));
}

void BooleanItem::ToProtoString(std::string& result)
{
	result.append(" ");
	result.append(fd_->name());
	result.append(": ");
	int v = checkbox_->isChecked();
	result.append(std::to_string(v));
}

ReaptedItem::ReaptedItem(Importer& importer, const FieldDescriptor* fd) : importer_(importer), fd_(fd)
{
	add_button_ = this->addWidget(std::make_unique<Wt::WPushButton>("+"));
	table_ = this->addWidget(std::make_unique<Wt::WTable>());
	// 添加子项的逻辑
	auto f = [this](){
		int index = items_.size();
		// 第一列放一个用于删除当前项的按钮
		auto button = std::make_unique<Wt::WPushButton>("-");
		auto button_ptr = button.get();
		button_ptr->setMargin(10.0);
		table_->elementAt(index, 0)->addWidget(std::move(button));
		// 第一列的删除按钮居中对齐
		Wt::WFlags<Wt::AlignmentFlag> align = Wt::WFlags<Wt::AlignmentFlag>(Wt::AlignmentFlag::Center|Wt::AlignmentFlag::Middle);
		table_->elementAt(index, 0)->setContentAlignment(align);
		auto item = AbstractProtoItem::Create(this->importer_, this->fd_, false);
		auto item_ptr = item.get();
		table_->elementAt(index, 1)->addWidget(std::move(item));
		items_.push_back(item_ptr);
		delete_buttons_.push_back(button_ptr);
		button_ptr->clicked().connect([this, button_ptr](){
			auto it = std::find(delete_buttons_.begin(), delete_buttons_.end(), button_ptr);
			if(it == delete_buttons_.end())
			{
				std::cout<<"======can not find button\n";
				return;
			}

			// 将要删除的index
			auto delete_index = std::distance(delete_buttons_.begin(), it);
			table_->removeRow(delete_index);
			delete_buttons_.erase(it);
			items_.erase(items_.begin() + delete_index);
		});
	};
	add_button_->clicked().connect(f);
}

void ReaptedItem::ToProtoString(std::string& result)
{
	for(AbstractProtoItem* item: items_)
	{
		result.append(" ");
		result.append(fd_->name());
		// result.append(": "); // repeated 不需要冒号也可以
		item->ToProtoString(result);
	}
}

void NumberItem::ToProtoString(std::string& result)
{
	result.append(" ");
	result.append(fd_->name());
	result.append(": ");
	result.append(input_box_->text().toUTF8());
}
