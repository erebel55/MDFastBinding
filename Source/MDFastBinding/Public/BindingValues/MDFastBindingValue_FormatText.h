﻿// Copyright Dylan Dumesnil. All Rights Reserved.

#pragma once

#include "MDFastBindingValueBase.h"
#include "MDFastBindingValue_FormatText.generated.h"

/**
 * Formats text based on the input format string and inputs
 */
UCLASS(meta = (DisplayName = "Format Text"))
class MDFASTBINDING_API UMDFastBindingValue_FormatText : public UMDFastBindingValueBase
{
	GENERATED_BODY()

public:
	virtual const FProperty* GetOutputProperty() override;

#if WITH_EDITORONLY_DATA
	virtual FText GetDisplayName() override;
#endif

protected:
	virtual TTuple<const FProperty*, void*> GetValue_Internal(UObject* SourceObject) override;
	virtual void SetupBindingItems() override;

	UPROPERTY(EditAnywhere, Category = "Binding")
	FText FormatText = INVTEXT("{InputString}");

private:
	FTextFormat TextFormat;

	UPROPERTY(Transient)
	FText OutputValue;

	UPROPERTY(Transient)
	TArray<FName> Arguments;

	const FProperty* TextProp = nullptr;

	FFormatNamedArguments Args;
};
