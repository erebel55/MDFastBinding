﻿// Copyright Dylan Dumesnil. All Rights Reserved.

#pragma once

#include "IMDFastBindingPropertySetter.h"
#include "Containers/Set.h"

class FFieldClass;
class FProperty;

/**
 *
 */
class MDFASTBINDING_API FMDFastBindingPropertySetter_Containers : public IMDFastBindingPropertySetter
{
public:
	FMDFastBindingPropertySetter_Containers();

	virtual const TSet<FFieldClass*>& GetSupportedFieldTypes() const override { return SupportedFieldTypes; }
	virtual void SetPropertyInContainer(const FProperty& DestinationProp, void* DestinationContainerPtr, const FProperty& SourceProp, const void* SourceValuePtr) const override;
	virtual void SetPropertyDirectly(const FProperty& DestinationProp, void* DestinationValuePtr, const FProperty& SourceProp, const void* SourceValuePtr) const override;
	virtual bool CanSetProperty(const FProperty& DestinationProp, const FProperty& SourceProp) const override;

private:
	TSet<FFieldClass*> SupportedFieldTypes;

};
