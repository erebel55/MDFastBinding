// Copyright Dylan Dumesnil. All Rights Reserved.

#include "MDFastBindingDesignerExtension.h"

#include "IHasDesignerExtensibility.h"
#include "MDFastBindingContainer.h"
#include "Util/MDFastBindingEditorConfig.h"
#include "WidgetBlueprint.h"
#include "WidgetBlueprintEditor.h"
#include "Blueprint/WidgetTree.h"
#include "Slate/SObjectWidget.h"
#include "Util/MDFastBindingEditorHelpers.h"
#include "WidgetExtension/MDFastBindingWidgetClassExtension.h"
#include "WidgetExtension/MDFastBindingWidgetExtension.h"

class FMDFastBindingDesignerExtensionFactory : public IDesignerExtensionFactory, public TSharedFromThis<FMDFastBindingDesignerExtensionFactory>
{
public:
	FMDFastBindingDesignerExtensionFactory() = default;
	virtual ~FMDFastBindingDesignerExtensionFactory() = default;
	
	virtual TSharedRef<FDesignerExtension> CreateDesignerExtension() const override
	{
		return MakeShared<FMDFastBindingDesignerExtension>();
	}
};

TSharedRef<IDesignerExtensionFactory> FMDFastBindingDesignerExtension::MakeFactory()
{
	return MakeShared<FMDFastBindingDesignerExtensionFactory>();
}

FMDFastBindingDesignerExtension::FMDFastBindingDesignerExtension()
{
	ExtensionId = TEXT("MDFastBinding");
}

void FMDFastBindingDesignerExtension::Initialize(IUMGDesigner* InDesigner, UWidgetBlueprint* InBlueprint)
{
	FDesignerExtension::Initialize(InDesigner, InBlueprint);

	GetDefault<UMDFastBindingEditorConfig>()->OnShouldRunBindingsAtDesignTimeChanged.AddSP(this, &FMDFastBindingDesignerExtension::OnShouldRunBindingsAtDesignTimeChanged);
}

void FMDFastBindingDesignerExtension::Uninitialize()
{
	TerminateBindingInstances();

	GetDefault<UMDFastBindingEditorConfig>()->OnShouldRunBindingsAtDesignTimeChanged.RemoveAll(this);

	FDesignerExtension::Uninitialize();
}

void FMDFastBindingDesignerExtension::Tick(const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime)
{
	UpdateBindingInstances();
}

void FMDFastBindingDesignerExtension::PreviewContentChanged(TSharedRef<SWidget> NewContent)
{
	if (NewContent->GetType() == TEXT("SObjectWidget"))
	{
		const TSharedRef<SObjectWidget> ObjectWidgetContent = StaticCastSharedRef<SObjectWidget>(NewContent);
		PreviewedWidget = ObjectWidgetContent->GetWidgetObject();
	}
	else
	{
		PreviewedWidget.Reset();
	}
	
	InitializeBindingInstances();
}

void FMDFastBindingDesignerExtension::InitializeBindingInstances()
{
	TerminateBindingInstances();
	
	if (!GetDefault<UMDFastBindingEditorConfig>()->ShouldRunBindingsAtDesignTime())
	{
		return;
	}

	// Initialize bindings for the preview widget and any subwidgets
	if (UUserWidget* PreviewWidget = GetPreviewWidget())
	{
		InitializeBindingInstanceForWidget(PreviewWidget);
	}
}

void FMDFastBindingDesignerExtension::UpdateBindingInstances()
{
	for (auto It = BindingContainers.CreateIterator(); It; ++It)
	{
		if (UUserWidget* BindingOwner = It.Key().Get())
		{
			for (const TStrongObjectPtr<UMDFastBindingContainer>& BindingContainer : It.Value())
			{
				BindingContainer->UpdateBindings(BindingOwner);
			}
		}
		else
		{
			It.Value().Reset();
			It.RemoveCurrent();
		}
	}
}

void FMDFastBindingDesignerExtension::TerminateBindingInstances()
{
	for (auto It = BindingContainers.CreateIterator(); It; ++It)
	{
		if (UUserWidget* BindingOwner = It.Key().Get())
		{
			for (const TStrongObjectPtr<UMDFastBindingContainer>& BindingContainer : It.Value())
			{
				BindingContainer->TerminateBindings(BindingOwner);
			}
		}
		
		It.Value().Reset();
	}

	BindingContainers.Reset();
}

void FMDFastBindingDesignerExtension::InitializeBindingInstanceForWidget(UUserWidget* Widget)
{
	if (Widget == nullptr)
	{
		return;
	}

	bool bDoesHaveExtensionBinding = false;
	// Widgets Extensions aren't initialized at design-time so we have to go to the class extension 
	if (UWidgetBlueprintGeneratedClass* BPClass = Cast<UWidgetBlueprintGeneratedClass>(Widget->GetClass()))
	{
		if (const UMDFastBindingWidgetClassExtension* Extension = BPClass->GetExtension<UMDFastBindingWidgetClassExtension>())
		{
			bDoesHaveExtensionBinding = Extension->HasBindings();
		}
		
		while (BPClass != nullptr)
		{
			BPClass->ForEachExtension([this, Widget](UWidgetBlueprintGeneratedClassExtension* Extension)
			{
				if (const UMDFastBindingWidgetClassExtension* SuperClassExtension = Cast<UMDFastBindingWidgetClassExtension>(Extension))
				{
					InitializeBindingContainerForWidget(SuperClassExtension->GetBindingContainer(), Widget);
				}
			});
			BPClass = Cast<UWidgetBlueprintGeneratedClass>(BPClass->GetSuperClass());
		}
	}

	// Fallback to checking for a legacy property-based binding container
	if (!bDoesHaveExtensionBinding)
	{
		if (const UMDFastBindingContainer* LegacyBindingContainer = MDFastBindingEditorHelpers::FindBindingContainerCDOInClass(Widget->GetClass()))
		{
			InitializeBindingContainerForWidget(LegacyBindingContainer, Widget);
		}
	}
		
	if (Widget->WidgetTree != nullptr)
	{
		Widget->WidgetTree->ForEachWidget([&](UWidget* SubWidget)
		{
			if (UUserWidget* UserWidget = Cast<UUserWidget>(SubWidget))
			{
				InitializeBindingInstanceForWidget(UserWidget);
			}
		});
	}
}

void FMDFastBindingDesignerExtension::InitializeBindingContainerForWidget(const UMDFastBindingContainer* CDOBindingContainer, UUserWidget* Widget)
{
	if (UMDFastBindingContainer* BindingContainer = DuplicateObject<UMDFastBindingContainer>(CDOBindingContainer, Widget))
	{
		BindingContainer->InitializeBindings(Widget);
		BindingContainers.FindOrAdd(Widget).AddUnique(TStrongObjectPtr<UMDFastBindingContainer>(BindingContainer));
	}
}

void FMDFastBindingDesignerExtension::OnShouldRunBindingsAtDesignTimeChanged()
{
	// Refresh to undo any changes our bindings might have done
	if (FWidgetBlueprintEditor* WidgetEditor = FindWidgetEditor())
	{
		WidgetEditor->RefreshPreview();
	}
}

UUserWidget* FMDFastBindingDesignerExtension::GetPreviewWidget() const
{
	return PreviewedWidget.Get();
}

FWidgetBlueprintEditor* FMDFastBindingDesignerExtension::FindWidgetEditor() const
{
	if (UWidgetBlueprint* WidgetBP = Blueprint.Get())
	{
		if (IAssetEditorInstance* AssetEditorInstance = GEditor->GetEditorSubsystem<UAssetEditorSubsystem>()->FindEditorForAsset(WidgetBP, false))
		{
			return static_cast<FWidgetBlueprintEditor*>(AssetEditorInstance);
		}
	}

	return nullptr;
}
