﻿//------------------------------------------------------------------------------
//     This code was generated by a tool.
//
//     Changes to this file may cause incorrect behavior and will be lost if
//     the code is regenerated.
//------------------------------------------------------------------------------
#include "pch.h"

#pragma warning(push)
#pragma warning(disable: 4100) // unreferenced formal parameter

#if defined _DEBUG && !defined DISABLE_XAML_GENERATED_BINDING_DEBUG_OUTPUT
extern "C" __declspec(dllimport) int __stdcall IsDebuggerPresent();
#endif

#include "Scenario2_GetRawData.xaml.h"

void ::SDKTemplate::Scenario2_GetRawData::InitializeComponent()
{
    if (_contentLoaded)
    {
        return;
    }
    _contentLoaded = true;
    ::Windows::Foundation::Uri^ resourceLocator = ref new ::Windows::Foundation::Uri(L"ms-appx:///Scenario2_GetRawData.xaml");
    ::Windows::UI::Xaml::Application::LoadComponent(this, resourceLocator, ::Windows::UI::Xaml::Controls::Primitives::ComponentResourceLocation::Application);
}

void ::SDKTemplate::Scenario2_GetRawData::Connect(int __connectionId, ::Platform::Object^ __target)
{
    switch (__connectionId)
    {
    case 1:
        {
            this->outputTextBlock = safe_cast<::Windows::UI::Xaml::Controls::TextBlock^>(__target);
        }
        break;
    case 2:
        {
            this->WideColumn3 = safe_cast<::Windows::UI::Xaml::Controls::ColumnDefinition^>(__target);
        }
        break;
    case 3:
        {
            this->WideColumn4 = safe_cast<::Windows::UI::Xaml::Controls::ColumnDefinition^>(__target);
        }
        break;
    case 4:
        {
            this->ColorFrameBlock = safe_cast<::Windows::UI::Xaml::Controls::Grid^>(__target);
        }
        break;
    case 5:
        {
            this->DepthFrameBlock = safe_cast<::Windows::UI::Xaml::Controls::Grid^>(__target);
        }
        break;
    case 6:
        {
            this->InfraredFrameBlock = safe_cast<::Windows::UI::Xaml::Controls::Grid^>(__target);
        }
        break;
    case 7:
        {
            this->depthFilterBlock = safe_cast<::Windows::UI::Xaml::Controls::Grid^>(__target);
        }
        break;
    case 8:
        {
            this->depthFilterImage = safe_cast<::Windows::UI::Xaml::Controls::Image^>(__target);
        }
        break;
    case 9:
        {
            this->infraredFrameImage = safe_cast<::Windows::UI::Xaml::Controls::Image^>(__target);
        }
        break;
    case 10:
        {
            this->depthFrameImage = safe_cast<::Windows::UI::Xaml::Controls::Image^>(__target);
        }
        break;
    case 11:
        {
            this->colorFrameImage = safe_cast<::Windows::UI::Xaml::Controls::Image^>(__target);
        }
        break;
    case 12:
        {
            this->WideColumn1 = safe_cast<::Windows::UI::Xaml::Controls::ColumnDefinition^>(__target);
        }
        break;
    case 13:
        {
            this->WideColumn2 = safe_cast<::Windows::UI::Xaml::Controls::ColumnDefinition^>(__target);
        }
        break;
    case 14:
        {
            this->ColorPreviewBlock = safe_cast<::Windows::UI::Xaml::Controls::Grid^>(__target);
        }
        break;
    case 15:
        {
            this->DepthPreviewBlock = safe_cast<::Windows::UI::Xaml::Controls::Grid^>(__target);
        }
        break;
    case 16:
        {
            this->InfraredPreviewBlock = safe_cast<::Windows::UI::Xaml::Controls::Grid^>(__target);
        }
        break;
    case 17:
        {
            this->infraredPreviewImage = safe_cast<::Windows::UI::Xaml::Controls::Image^>(__target);
        }
        break;
    case 18:
        {
            this->depthPreviewImage = safe_cast<::Windows::UI::Xaml::Controls::Image^>(__target);
        }
        break;
    case 19:
        {
            this->colorPreviewImage = safe_cast<::Windows::UI::Xaml::Controls::Image^>(__target);
        }
        break;
    case 20:
        {
            this->NextButton = safe_cast<::Windows::UI::Xaml::Controls::Button^>(__target);
            (safe_cast<::Windows::UI::Xaml::Controls::Button^>(this->NextButton))->Click += ref new ::Windows::UI::Xaml::RoutedEventHandler(this, (void (::SDKTemplate::Scenario2_GetRawData::*)
                (::Platform::Object^, ::Windows::UI::Xaml::RoutedEventArgs^))&Scenario2_GetRawData::NextButton_Click);
        }
        break;
    case 21:
        {
            this->captureButton = safe_cast<::Windows::UI::Xaml::Controls::Button^>(__target);
            (safe_cast<::Windows::UI::Xaml::Controls::Button^>(this->captureButton))->Click += ref new ::Windows::UI::Xaml::RoutedEventHandler(this, (void (::SDKTemplate::Scenario2_GetRawData::*)
                (::Platform::Object^, ::Windows::UI::Xaml::RoutedEventArgs^))&Scenario2_GetRawData::captureButton_Click);
        }
        break;
    }
    _contentLoaded = true;
}

::Windows::UI::Xaml::Markup::IComponentConnector^ ::SDKTemplate::Scenario2_GetRawData::GetBindingConnector(int __connectionId, ::Platform::Object^ __target)
{
    __connectionId;         // unreferenced
    __target;               // unreferenced
    return nullptr;
}

#pragma warning(pop)

