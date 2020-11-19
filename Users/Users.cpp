#include <stdio.h>

#include <Application.h>
#include <MessageRunner.h>
#include <Invoker.h>
#include <Window.h>
#include <Alert.h>
#include <View.h>
#include <Menu.h>
#include <MenuBar.h>
#include <TabView.h>
#include <LayoutBuilder.h>
#include <Rect.h>
#include <IconUtils.h>
#include <Resources.h>

#include <private/interface/ColumnListView.h>
#include <private/interface/ColumnTypes.h>

#include <pwd.h>
#include <grp.h>

#include <private/kernel/util/KMessage.h>

#include "Resources.h"
#include "UserForm.h"
#include "UserDB.h"


enum {
	updateMsg = 1,
	addMsg,
	removeMsg,
	editMsg,
	setPasswordMsg,
};

enum {
	userNameCol = 0,
	userIdCol,
	userGroupIdCol,
	userPasswdCol,
	userDirCol,
	userShellCol,
	userGecosCol,
};

enum {
	groupNameCol = 0,
	groupIdCol,
	groupPasswdCol,
	groupMembersCol,
};


static BRow *FindIntRow(BColumnListView *view, int32 col, BRow *parent, int32 val)
{
	BRow *row;
	BString name;
	for (int32 i = 0; i < view->CountRows(parent); i++) {
		row = view->RowAt(i, parent);
		if (((BIntegerField*)row->GetField(col))->Value() == val)
			return row;
	}
	return NULL;
}


static void ListUsers(BColumnListView *view)
{
	BRow *row;
	BList prevRows;

	int32 count;
	passwd** entries;

	KMessage reply;
	if (GetUsers(reply, count, entries) < B_OK) count = 0;

	for (int32 i = 0; i < view->CountRows(); i++) {
		row = view->RowAt(i);
		prevRows.AddItem((void*)(addr_t)(((BIntegerField*)row->GetField(userIdCol))->Value()));
	}

	for (int32 i = 0; i < count; i++) {
		passwd *entry = entries[i];

		prevRows.RemoveItem((void*)(addr_t)entry->pw_uid);
		row = FindIntRow(view, userIdCol, NULL, entry->pw_uid);
		if (row == NULL) {
			row = new BRow();
			view->AddRow(row);
		}

		row->SetField(new BStringField(entry->pw_name), userNameCol);
		row->SetField(new BIntegerField(entry->pw_uid), userIdCol);
		row->SetField(new BIntegerField(entry->pw_gid), userGroupIdCol);
		row->SetField(new BStringField(entry->pw_passwd), userPasswdCol);
		row->SetField(new BStringField(entry->pw_dir), userDirCol);
		row->SetField(new BStringField(entry->pw_shell), userShellCol);
		row->SetField(new BStringField(entry->pw_gecos), userGecosCol);
	}

	for (int32 i = 0; i < prevRows.CountItems(); i++) {
		row = FindIntRow(view, userIdCol, NULL, (int32)(addr_t)prevRows.ItemAt(i));
		view->RemoveRow(row);
		delete row;
	}
}

static BColumnListView *NewUsersView()
{
	BColumnListView *view;
	view = new BColumnListView("Users", B_NAVIGABLE);
	view->SetInvocationMessage(new BMessage(editMsg));
	view->AddColumn(new BStringColumn("Name", 96, 50, 500, B_TRUNCATE_END), userNameCol);
	view->AddColumn(new BIntegerColumn("ID", 64, 32, 128, B_ALIGN_RIGHT), userIdCol);
	view->AddColumn(new BIntegerColumn("Group ID", 64, 32, 128, B_ALIGN_RIGHT), userGroupIdCol);
	view->AddColumn(new BStringColumn("passwd", 48, 50, 500, B_TRUNCATE_END), userPasswdCol);
	view->AddColumn(new BStringColumn("Home directory", 150, 50, 500, B_TRUNCATE_END), userDirCol);
	view->AddColumn(new BStringColumn("Shell", 150, 50, 500, B_TRUNCATE_END), userShellCol);
	view->AddColumn(new BStringColumn("Real name", 150, 50, 500, B_TRUNCATE_END), userGecosCol);
	ListUsers(view);
	return view;
}

static void ListGroups(BColumnListView *view)
{
	BRow *row;
	BList prevRows;

	int32 count;
	group** entries;

	KMessage reply;
	if (GetGroups(reply, count, entries) < B_OK) count = 0;

	for (int32 i = 0; i < view->CountRows(); i++) {
		row = view->RowAt(i);
		prevRows.AddItem((void*)(addr_t)(((BIntegerField*)row->GetField(groupIdCol))->Value()));
	}

	for (int32 i = 0; i < count; i++) {
		group* entry = entries[i];

		prevRows.RemoveItem((void*)(addr_t)entry->gr_gid);
		row = FindIntRow(view, userIdCol, NULL, entry->gr_gid);
		if (row == NULL) {
			row = new BRow();
			view->AddRow(row);
		}

		row->SetField(new BStringField(entry->gr_name), groupNameCol);
		row->SetField(new BIntegerField(entry->gr_gid), groupIdCol);
		row->SetField(new BStringField(entry->gr_passwd), groupPasswdCol);

		BString str;
		for (int j = 0; entry->gr_mem[j] != NULL; j++) {
			if (j > 0) str += ", ";
			str += entry->gr_mem[j];
		}
		row->SetField(new BStringField(str), groupMembersCol);
	}

	for (int32 i = 0; i < prevRows.CountItems(); i++) {
		row = FindIntRow(view, groupIdCol, NULL, (int32)(addr_t)prevRows.ItemAt(i));
		view->RemoveRow(row);
		delete row;
	}
}

static BColumnListView *NewGroupsView()
{
	BColumnListView *view;
	view = new BColumnListView("Groups", B_NAVIGABLE);
	view->SetInvocationMessage(new BMessage(editMsg));
	view->AddColumn(new BStringColumn("Name", 96, 50, 500, B_TRUNCATE_END), groupNameCol);
	view->AddColumn(new BIntegerColumn("ID", 64, 32, 128, B_ALIGN_RIGHT), groupIdCol);
	view->AddColumn(new BStringColumn("passwd", 48, 50, 500, B_TRUNCATE_END), groupPasswdCol);
	view->AddColumn(new BStringColumn("Members", 128, 50, 500, B_TRUNCATE_END), groupMembersCol);
	ListGroups(view);
	return view;
}


class IconMenuItem: public BMenuItem
{
public:
	IconMenuItem(BBitmap *bitmap, BMessage* message, char shortcut = 0, uint32 modifiers = 0):
		BMenuItem("", message, shortcut, modifiers),
		fBitmap(bitmap)
	{
	}

	~IconMenuItem()
	{
		if (fBitmap != NULL) {
			delete fBitmap; fBitmap = NULL;
		}
	}

	void GetContentSize(float* width, float* height)
	{
		if (fBitmap == NULL) {
			*width = 0;
			*height = 0;
			return;
		}
		*width = fBitmap->Bounds().Width() + 1;
		*height = fBitmap->Bounds().Height() + 1;
	};

	void DrawContent()
	{
		if (fBitmap == NULL)
			return;

		BRect frame = Frame();
		BPoint center = BPoint((frame.left + frame.right)/2, (frame.top + frame.bottom)/2);
		Menu()->PushState();
		Menu()->SetDrawingMode(B_OP_ALPHA);
		Menu()->DrawBitmap(fBitmap, center - BPoint(fBitmap->Bounds().Width()/2, fBitmap->Bounds().Height()/2));
		Menu()->PopState();
	}

private:
	BBitmap *fBitmap;
};


BBitmap *LoadIcon(int32 id, int32 width, int32 height)
{
	size_t dataSize;
	const void* data = BApplication::AppResources()->FindResource(B_VECTOR_ICON_TYPE, id, &dataSize);

	if (data == NULL) return NULL;

	BBitmap *bitmap = new BBitmap(BRect(0, 0, width - 1, height - 1), B_RGBA32);

	if (bitmap == NULL) return NULL;

	if (BIconUtils::GetVectorIcon((const uint8*)data, dataSize, bitmap) != B_OK) {
		delete bitmap;
		return NULL;
	}

	return bitmap;
}


class TestWindow: public BWindow
{
private:
	BMessageRunner fListUpdater;
	BTabView *fTabView;
	BColumnListView *fUsersView;
	BColumnListView *fGroupsView;

public:
	TestWindow(BRect frame): BWindow(frame, "Users", B_DOCUMENT_WINDOW_LOOK, B_NORMAL_WINDOW_FEEL, B_ASYNCHRONOUS_CONTROLS),
		fListUpdater(BMessenger(this), BMessage(updateMsg), 500000)
	{
		BMenuBar *menuBar = new BMenuBar("menu", B_ITEMS_IN_ROW, true);
		BLayoutBuilder::Menu<>(menuBar)
/*
			.AddMenu(new BMenu("File"))
				.AddItem(new BMenuItem("Close", new BMessage(B_QUIT_REQUESTED), 'W'))
				.End()
*/
			.AddMenu(new BMenu("Action"))
				.AddItem(new BMenuItem("Add", new BMessage(addMsg)))
				.AddItem(new BMenuItem("Remove", new BMessage(removeMsg)))
				.AddItem(new BMenuItem("Edit", new BMessage(editMsg)))
				.AddItem(new BMenuItem("Set password", new BMessage(setPasswordMsg)))
				.End()
			.End()
		;

		BMenuBar *toolBar = new BMenuBar("toolbar", B_ITEMS_IN_ROW, true);
		BLayoutBuilder::Menu<>(toolBar)
			.AddItem(new IconMenuItem(LoadIcon(resAddIcon, 16, 16), new BMessage(addMsg)))
			.AddItem(new IconMenuItem(LoadIcon(resRemoveIcon, 16, 16), new BMessage(removeMsg)))
			.AddItem(new IconMenuItem(LoadIcon(resEditIcon, 16, 16), new BMessage(editMsg)))
			.End()
		;

		fTabView = new BTabView("tabView", B_WIDTH_FROM_LABEL);
		fTabView->SetBorder(B_NO_BORDER);

		BTab *tab = new BTab(); fTabView->AddTab(fUsersView = NewUsersView(), tab);
		tab = new BTab(); fTabView->AddTab(fGroupsView = NewGroupsView(), tab);

		BLayoutBuilder::Group<>(this, B_VERTICAL, 0)
			.Add(menuBar)
			.Add(toolBar)
			.AddGroup(B_VERTICAL, 0)
				.Add(fTabView)
				.SetInsets(-1, 0, -1, -1)
				.End()
			.End()
		;

		SetKeyMenuBar(menuBar);
		fUsersView->MakeFocus();
	}

	void MessageReceived(BMessage *msg)
	{
		switch (msg->what) {
		case updateMsg: {
			ListUsers(fUsersView);
			return;
		}
		case addMsg: {
			BTab *tab = fTabView->TabAt(fTabView->Selection());
			if (tab == NULL) return;
			BView *view = tab->View();
			if (view == fUsersView) {
				ShowUserForm(-1, BPoint((Frame().left + Frame().right)/2, (Frame().top + Frame().bottom)/2));
			} else if (view == fGroupsView) {
				BAlert *alert = new BAlert("Users", "Adding new group feature is not yet implemented.", "OK", NULL, NULL, B_WIDTH_AS_USUAL, B_WARNING_ALERT);
				alert->Go(NULL);
				return;
			}
			return;
		}
		case removeMsg: {
			BTab *tab = fTabView->TabAt(fTabView->Selection());
			if (tab == NULL) return;
			BView *view = tab->View();
			if (view == fUsersView) {
				BRow *row = fUsersView->CurrentSelection(NULL);
				if (row == NULL) return;
				const char *userName = ((BStringField*)row->GetField(userNameCol))->String();

				int32 which;
				BInvoker *invoker = NULL;
				if (msg->FindInt32("which", &which) < B_OK) {
					BString str;
					str.SetToFormat("Are you sure you want to delete user \"%s\"?", userName);
					BAlert *alert = new BAlert("Users", str, "Yes", "No", NULL, B_WIDTH_AS_USUAL, B_WARNING_ALERT);
					invoker = new BInvoker(new BMessage(*msg), this);
					invoker->Message()->SetPointer("invoker", invoker);
					alert->Go(invoker);
					return;
				} else {
					if (msg->FindPointer("invoker", &(void*&)invoker) >= B_OK) {
						delete invoker; invoker = NULL;
					}
					if (which != 0) return;
				}
				if (DeleteUser(-1, userName) < B_OK) return;
				ListUsers(fUsersView);
			} else if (view == fGroupsView) {
				BRow *row = fGroupsView->CurrentSelection(NULL);
				if (row == NULL) return;
				const char *groupName = ((BStringField*)row->GetField(groupNameCol))->String();

				int32 which;
				BInvoker *invoker = NULL;
				if (msg->FindInt32("which", &which) < B_OK) {
					BString str;
					str.SetToFormat("Are you sure you want to delete group \"%s\"?", groupName);
					BAlert *alert = new BAlert("Users", str, "Yes", "No", NULL, B_WIDTH_AS_USUAL, B_WARNING_ALERT);
					invoker = new BInvoker(new BMessage(*msg), this);
					invoker->Message()->SetPointer("invoker", invoker);
					alert->Go(invoker);
					return;
				} else {
					if (msg->FindPointer("invoker", &(void*&)invoker) >= B_OK) {
						delete invoker; invoker = NULL;
					}
					if (which != 0) return;
				}
				if (DeleteGroup(-1, groupName) < B_OK) return;
				ListGroups(fGroupsView);
			}
			return;
		}
		case editMsg: {
			BTab *tab = fTabView->TabAt(fTabView->Selection());
			if (tab == NULL) return;
			BView *view = tab->View();
			if (view == fUsersView) {
				BRow *row = fUsersView->CurrentSelection(NULL);
				if (row == NULL) return;
				int32 uid = ((BIntegerField*)row->GetField(userIdCol))->Value();
				ShowUserForm(uid, BPoint((Frame().left + Frame().right)/2, (Frame().top + Frame().bottom)/2));
			} else if (view == fGroupsView) {
				BAlert *alert = new BAlert("Users", "Editing group feature is not yet implemented.", "OK", NULL, NULL, B_WIDTH_AS_USUAL, B_WARNING_ALERT);
				alert->Go(NULL);
				return;
			}
			return;
		}
		case setPasswordMsg: {
			BAlert *alert = new BAlert("Users", "Set password feature is not yet implemented.", "OK", NULL, NULL, B_WIDTH_AS_USUAL, B_WARNING_ALERT);
			alert->Go(NULL);
			return;
		}
		}
		BWindow::MessageReceived(msg);
	}

	void Quit()
	{
		be_app_messenger.SendMessage(B_QUIT_REQUESTED);
		BWindow::Quit();
	}

};

class TestApplication: public BApplication
{
private:
	TestWindow *fWnd;

public:
	TestApplication(): BApplication("application/x-vnd.Test-Users")
	{
	}

	void ReadyToRun() {
		fWnd = new TestWindow(BRect(0, 0, 512 + 256, 256).OffsetByCopy(64, 64));
		fWnd->Show();
	}
};


int main()
{
	TestApplication app;
	app.Run();
	return 0;
}
