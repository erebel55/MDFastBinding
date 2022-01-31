﻿#pragma once

#include "WorkflowOrientedApp/WorkflowTabFactory.h"

class UMDFastBindingContainer;
class UMDFastBindingDestinationBase;
class FBlueprintEditor;
class FMenuBuilder;
class IDetailsView;
class SMDFastBindingEditorGraphWidget;

class FMDBindingEditorContainerSelectMenuNode : public TSharedFromThis<FMDBindingEditorContainerSelectMenuNode>
{
public:
	FText DisplayName;

	TWeakObjectPtr<UStruct> NodeClass;
	
	TWeakObjectPtr<UMDFastBindingContainer> BindingContainer;

	TArray<TSharedRef<FMDBindingEditorContainerSelectMenuNode>> Children;
};

class SMDFastBindingEditorWidget : public SCompoundWidget, public FSelfRegisteringEditorUndoClient
{
public:

	SLATE_BEGIN_ARGS(SMDFastBindingEditorWidget){}
	SLATE_END_ARGS();

	void Construct(const FArguments&, const TWeakPtr<FBlueprintEditor> InBlueprintEditor);
	void AssignBindingData(UClass* BindingOwnerClass);
	void SelectBindingContainer(UMDFastBindingContainer* BindingContainer);
	void SelectBindingContainer(TWeakObjectPtr<UMDFastBindingContainer> BindingContainer);
	void SelectBinding(UMDFastBindingDestinationBase* InBinding);
	void SelectBinding(TWeakObjectPtr<UMDFastBindingDestinationBase> InBinding, ESelectInfo::Type SelectionType);
	UMDFastBindingContainer* GetSelectedBindingContainer() const;
	UMDFastBindingDestinationBase* GetSelectedBinding() const;
	
	virtual void PostUndo(bool bSuccess) override;
	virtual void PostRedo(bool bSuccess) override;

	void RefreshGraph() const;

private:
	void OnGraphSelectionChanged(const FGraphPanelSelectionSet& Selection);
	
	void FillSelectMenu_Root(FMenuBuilder& MenuBuilder);
	void FillSelectMenu(FMenuBuilder& MenuBuilder, TSharedRef<FMDBindingEditorContainerSelectMenuNode> BindingNode);
	
	EVisibility GetContainerSelectorVisibility() const;
	EVisibility GetBindingSelectorVisibility() const;
	EVisibility GetBindingTreeVisibility() const;
	
	void PopulateBindingsList();
	TSharedRef<ITableRow> GenerateBindingListWidget(TWeakObjectPtr<UMDFastBindingDestinationBase> Binding, const TSharedRef<STableViewBase>& OwnerTable);
	
	FReply OnAddBinding();
	FReply OnDuplicateBinding(TWeakObjectPtr<UMDFastBindingDestinationBase> Binding);
	FReply OnDeleteBinding(TWeakObjectPtr<UMDFastBindingDestinationBase> Binding);

	void OnDetailsPanelPropertyChanged(const FPropertyChangedEvent& Event);
	
	TWeakObjectPtr<UMDFastBindingContainer> SelectedBindingContainer;
	TSharedPtr<FMDBindingEditorContainerSelectMenuNode> RootBindingNode;
	TArray<TWeakObjectPtr<UMDFastBindingContainer>> BindingContainers;
	TWeakObjectPtr<UMDFastBindingDestinationBase> SelectedBinding;
	TArray<TWeakObjectPtr<UMDFastBindingDestinationBase>> Bindings;
	TSharedPtr<IDetailsView> DetailsView;
	TSharedPtr<SWidgetSwitcher> DetailSwitcher;
	
	TSharedPtr<SListView<TWeakObjectPtr<UMDFastBindingDestinationBase>>> BindingListView;
	TSharedPtr<SMDFastBindingEditorGraphWidget> BindingGraphWidget;
};

struct FMDFastBindingEditorSummoner : public FWorkflowTabFactory
{
	FMDFastBindingEditorSummoner(TSharedPtr<FBlueprintEditor> BlueprintEditor);
	
	virtual const FSlateBrush* GetTabIcon(const FWorkflowTabSpawnInfo& Info) const override;
	virtual const FSlateIcon& GetTabSpawnerIcon(const FWorkflowTabSpawnInfo& Info) const override;
	virtual TSharedRef<SWidget> CreateTabBody(const FWorkflowTabSpawnInfo& Info) const override;

	static const FName TabId;

protected:
	TWeakPtr<FBlueprintEditor> WeakBlueprintEditor;

	FSlateIcon TabIcon;
};
