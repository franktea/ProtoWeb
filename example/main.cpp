/*
 * main.cpp
 *
 *  Created on: Jun 3, 2019
 *      Author: frank
 */

#include <Wt/WApplication.h>
#include <Wt/WIconPair.h>
#include <Wt/WText.h>
#include <Wt/WTree.h>
#include <Wt/WTreeNode.h>
#include <Wt/WBootstrapTheme.h>
#include <boost/filesystem.hpp>
#include <string>
#include "proto_web.h"


std::string& StringFormat(std::string& buff, const std::string fmt_str, ...)
{
    size_t n = 256;

    if (buff.size() < n)
    {
        buff.resize(n);
    }
    else
    {
        n = buff.size();
    }

    //cout<<"------------------------"<<endl;
    while (1)
    {
        //std::cout<<"processing...., n="<<n<<std::endl;
        va_list ap;
        va_start(ap, fmt_str);
        const int final_n = vsnprintf(&buff[0], n, fmt_str.c_str(), ap);
        va_end(ap);
        if (final_n < 0)
        {
            //n += size_t(-final_n);
            buff = "encoding error";
            break;
        }

        if (static_cast<size_t>(final_n) >= n)
        {
            n += static_cast<size_t>(final_n)-n + 1;
            if (n > 4096)
            {
                buff = "string too long, larger then 4096...";
                break;
            }
            buff.resize(n);
        }
        else
        {
            buff[final_n] = '\0';
            buff.resize(final_n);
            break;
        }
    }

    return buff;

}

class MyMultiFileErrorCollector: public google::protobuf::compiler::MultiFileErrorCollector
{
public:
    virtual void AddError(const std::string& filename, int line, int column, const std::string& message)
    {
        std::string str;
        errors_.insert(
                std::pair<std::string, std::string>(
                        filename + StringFormat(str, "(line=%d, col=%d)", line, column),
                        message));
    }

    void PrintError()
    {
        for(auto i = errors_.begin(); i != errors_.end(); ++i)
        {
            std::cout << "file " << i->first << ", msg: " << i->second << std::endl;
        }
    }
private:
    std::map<std::string, std::string> errors_;
};

class AllImporter
{
public:
	static Importer& GetImporter()
	{
		static AllImporter* aim = new AllImporter();
		return *aim->importer_;
	}
private:
	AllImporter()
	{
		src_tree_.MapPath("google/protobuf", "/usr/local/Cellar/protobuf/3.7.1/include/google/protobuf");
		src_tree_.MapPath("", "./");
		src_tree_.MapPath("", "./protos");
		importer_ = new Importer(&src_tree_, &mfec_);

		// 预加载全部的proto文件
		importer_->Import("tutorial.proto");
		importer_->Import("my_helloworld.proto");
	}

	DiskSourceTree src_tree_;
	MyMultiFileErrorCollector mfec_;
	Importer* importer_;
};

class ExampleApplication : public Wt::WApplication
{
public:
	ExampleApplication(const Wt::WEnvironment& env) :
		ui_(nullptr),
		text_(nullptr),
		Wt::WApplication(env)
	{
/*   +----------+-------------------------------------------------+
 *   |          |                                                 |
 *   |          |                                                 |
 *   |          |                                                 |
 *   |          |                                                 |
 *   |tree_panel|                   ui_panel                      |
 *   |          |                                                 |
 *   |          |                                                 |
 *   |          |                                                 |
 *   |          +-------------------------------------------------+
 *   |          |                   button                        |
 *   |          +-------------------------------------------------+
 *   |          |            generated_proto_panel                |
 *   |          |                                                 |
 *   |          |                                                 |
 *   +----------+-------------------------------------------------+
 */
		auto bootstrap = std::make_shared<Wt::WBootstrapTheme>();
		bootstrap->setVersion(Wt::BootstrapVersion::v3);
		this->setTheme(bootstrap);

		auto container = root()->addWidget(std::make_unique<Wt::WTable>());
		container->setStyleClass("table");
		root()->setHeight("100%");
		container->setHeight("100%");

		auto tree = std::make_unique<Wt::WTree>();
		tree->setSelectionMode(Wt::SelectionMode::Extended);
		auto folderIcon = std::make_unique<Wt::WIconPair>("icons/yellow-folder-closed.png",
					"icons/yellow-folder-open.png", false);
		auto node = std::make_unique<Wt::WTreeNode>("protos", std::move(folderIcon));
		Wt::WTreeNode* root_node = node.get();
		tree->setTreeRoot(std::move(node));
		tree->treeRoot()->label()->setTextFormat(Wt::TextFormat::Plain);
		tree->treeRoot()->setLoadPolicy(Wt::ContentLoading::NextLevel);
		tree->treeRoot()->expand();

		// 每个子节点被选中时的回调函数，创建该子节点对应的message的ui面板
		auto on_selected = [this](const std::string& message_name) {
			if(ui_)
			{
				ui_container_->removeWidget(ui_);
				ui_ = nullptr;
			}
			auto ui = AbstractProtoItem::Create(AllImporter::GetImporter(), message_name, true); // 带有名字空间的message全名是将名字空间用"."隔开
			ui_ = ui.get();
			ui_container_->addWidget(std::move(ui));
			if(text_)
			{
				text_->setText("");
			}
		};

		auto create_child_node = [this, &on_selected, root_node](std::string file_name, std::string message_name){
			auto icon = std::make_unique<Wt::WIconPair>("icons/git-blob.png", "icons/git-blob.png", false);
			auto proto_node = root_node->addChildNode(std::make_unique<Wt::WTreeNode>(file_name, std::move(icon)));
			proto_node->selected().connect(std::bind(on_selected, message_name));
		};

		create_child_node("my_helloworld.proto", "helloworld.AddRequest");
		create_child_node("tutorial.proto", "tutorial.Person");

		auto tree_panel = std::make_unique<Wt::WContainerWidget>();
		tree_panel->addWidget(std::move(tree));
		container->elementAt(0, 0)->addWidget(std::move(tree_panel));
		container->elementAt(0, 0)->setRowSpan(2);
		container->elementAt(0, 0)->setWidth("20%");

		auto ui_panel = std::make_unique<Wt::WContainerWidget>();
		ui_container_ = container->elementAt(0, 1)->addWidget(std::move(ui_panel));
		//container->elementAt(0, 1)->setHeight("100%");

		auto generated_proto_container = std::make_unique<Wt::WContainerWidget>();
		auto c = generated_proto_container.get();
		container->elementAt(1, 1)->addWidget(std::move(generated_proto_container));
		//container->elementAt(1, 1)->setWidth("100%");
		//container->elementAt(1, 1)->setHeight("20%");
	    Wt::WPushButton* submit = c->addWidget(std::make_unique<Wt::WPushButton>("Generate proto with input containt"));
	    c->addWidget(std::make_unique<Wt::WBreak>());
	    text_ = c->addWidget(std::make_unique<Wt::WTextArea>());
	    text_->setWidth("100%");

	    submit->clicked().connect([this](){
	    	if(!ui_)
	    		return;

	    	std::string proto_string;
	    	ui_->ToProtoString(proto_string);
	    	text_->setText(proto_string);
	    });
	}
private:
	AbstractProtoItem* ui_;
	Wt::WContainerWidget* ui_container_;
	Wt::WTextArea* text_;
};

int main(int argc, char **argv)
{
    return Wt::WRun(argc, argv, [](const Wt::WEnvironment& env) {
      return std::make_unique<ExampleApplication>(env);
    });
}


