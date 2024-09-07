﻿// Copyright Dylan Dumesnil. All Rights Reserved.

#include "MDFastBindingEditorStyle.h"

#include "Interfaces/IPluginManager.h"
#include "Styling/AppStyle.h"
#include "Styling/SlateStyleRegistry.h"
#include "Styling/StyleColors.h"
#include "Styling/SlateTypes.h"

TSharedPtr<FSlateStyleSet> FMDFastBindingEditorStyle::StyleInstance = nullptr;

void FMDFastBindingEditorStyle::Initialize()
{
	if (!StyleInstance.IsValid())
	{
		StyleInstance = Create();
		FSlateStyleRegistry::RegisterSlateStyle(*StyleInstance);
	}
}

void FMDFastBindingEditorStyle::Shutdown()
{
	FSlateStyleRegistry::UnRegisterSlateStyle(*StyleInstance);
	ensure(StyleInstance.IsUnique());
	StyleInstance.Reset();
}

const ISlateStyle& FMDFastBindingEditorStyle::Get()
{
	return *StyleInstance;
}

FName FMDFastBindingEditorStyle::GetStyleSetName()
{
	return TEXT("MDFastBindingEditorStyle");
}

#define IMAGE_BRUSH( RelativePath, ... ) FSlateImageBrush( Style->RootToContentDir(RelativePath, TEXT(".png")), __VA_ARGS__ )
#define BOX_BRUSH( RelativePath, ... ) FSlateBoxBrush( Style->RootToContentDir(RelativePath, TEXT(".png")), __VA_ARGS__ )
#define BORDER_BRUSH( RelativePath, ... ) FSlateBorderBrush( Style->RootToContentDir(RelativePath, TEXT(".png")), __VA_ARGS__ )
#define DEFAULT_FONT(...) FCoreStyle::GetDefaultFontStyle(__VA_ARGS__)

const FVector2D Icon16x16(16.0f, 16.0f);
const FVector2D Icon24x24(24.0f, 24.0f);
const FVector2D Icon32x32(32.0f, 32.0f);
const FVector2D Icon40x40(40.0f, 40.0f);

TSharedRef<FSlateStyleSet> FMDFastBindingEditorStyle::Create()
{
	TSharedRef<FSlateStyleSet> Style = MakeShareable(new FSlateStyleSet(GetStyleSetName()));
	Style->SetContentRoot(IPluginManager::Get().FindPlugin(TEXT("MDFastBinding"))->GetBaseDir() / TEXT("Resources"));
	Style->Set(TEXT("Icon.FastBinding_16x"), new IMAGE_BRUSH(TEXT("FastBindingIcon_16x"), Icon16x16));
	Style->Set(TEXT("Icon.FastBinding_24x"), new IMAGE_BRUSH(TEXT("FastBindingIcon_24x"), Icon24x24));
	Style->Set(TEXT("Icon.Clock"), new IMAGE_BRUSH(TEXT("ClockIcon_16x"), Icon16x16));
	Style->Set(TEXT("Icon.Flame"), new IMAGE_BRUSH(TEXT("FlameIcon_16x"), Icon16x16));
	Style->Set(TEXT("Icon.Disabled"), new IMAGE_BRUSH(TEXT("DisabledIcon"), Icon40x40));
	Style->Set(TEXT("Icon.Enabled"), new IMAGE_BRUSH(TEXT("EnabledIcon"), Icon40x40));
	Style->Set(TEXT("Icon.UpdateType.Once"), new IMAGE_BRUSH(TEXT("UpdateTypeOnceIcon_32x"), Icon32x32));
	Style->Set(TEXT("Icon.UpdateType.EventBased"), new IMAGE_BRUSH(TEXT("UpdateTypeEventBasedIcon_32x"), Icon32x32));
	Style->Set(TEXT("Icon.UpdateType.IfUpdatesNeeded"), new IMAGE_BRUSH(TEXT("UpdateTypeIfUpdatesNeededIcon_32x"), Icon32x32));
	Style->Set(TEXT("Icon.UpdateType.Always"), new IMAGE_BRUSH(TEXT("UpdateTypeAlwaysIcon_32x"), Icon32x32));

	Style->Set(TEXT("Background.Selector"), new FSlateColorBrush(FStyleColors::Select.GetSpecifiedColor() * 0.5f));
	Style->Set(TEXT("Background.SelectorInactive"), new FSlateColorBrush(FLinearColor::Transparent));
	
	FButtonStyle ButtonStyle = FAppStyle::Get().GetWidgetStyle< FButtonStyle >("FlatButton");
	ButtonStyle.SetNormalPadding(FMargin(2.f));
	ButtonStyle.SetPressedPadding(FMargin(2.f));
	Style->Set(TEXT("BindingButton"), ButtonStyle);

	Style->Set(TEXT("NodeTitleColor"), FLinearColor(FColorList::Cyan));
	Style->Set(TEXT("DestinationNodeTitleColor"), FLinearColor::Green);
	Style->Set(TEXT("InactiveDestinationNodeTitleColor"), FLinearColor::Green * 0.125f);
	Style->Set(TEXT("InvalidPinColor"), FLinearColor(FColorList::OrangeRed));

	return Style;
}

#undef IMAGE_BRUSH
#undef BOX_BRUSH
#undef BORDER_BRUSH
#undef DEFAULT_FONT
