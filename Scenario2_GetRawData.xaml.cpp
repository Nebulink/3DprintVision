#include "pch.h"
#include <windowsnumerics.h>
#include "Scenario2_GetRawData.xaml.h"
#include "FrameRenderer.h"

using namespace SDKTemplate;

using namespace concurrency;
using namespace Platform;
using namespace Platform::Collections;
using namespace Windows::Media::Devices::Core;
using namespace Windows::Foundation;
using namespace Windows::Foundation::Collections;
using namespace Windows::Foundation::Numerics;
using namespace Windows::Graphics::Imaging;
using namespace Windows::Media::Capture;
using namespace Windows::Media::Capture::Frames;
using namespace Windows::Perception::Spatial;
using namespace Windows::UI::Xaml::Media::Imaging;

// Used to determine whether a source has a Perception major type.
static String^ PerceptionMediaType = L"Perception";

// Returns the values from a std::map as a std::vector.
template<typename K, typename T>
static inline std::vector<T> values(std::map<K, T> const& inputMap)
{
	std::vector<T> outputVector(inputMap.size());
	std::transform(inputMap.begin(), inputMap.end(), outputVector.begin(), [](auto const& pair)
	{
		return pair.second;
	});
	return outputVector;
}

Scenario2_GetRawData::Scenario2_GetRawData() : rootPage(MainPage::Current)
{
	InitializeComponent();

	m_logger = ref new SimpleLogger(outputTextBlock);

	m_colorFrameRenderer = std::make_unique<FrameRenderer>(colorPreviewImage);
	m_depthFrameRenderer = std::make_unique<FrameRenderer>(depthPreviewImage);
	m_infraredFrameRenderer = std::make_unique<FrameRenderer>(infraredPreviewImage);

	m_singleColorFrameRenderer = std::make_unique<FrameRenderer>(colorFrameImage);
	m_singleDepthFrameRenderer = std::make_unique<FrameRenderer>(depthFrameImage);
	m_singleInfraredFrameRenderer = std::make_unique<FrameRenderer>(infraredFrameImage);

	m_depthFilterFrameRenderer = std::make_unique<FrameRenderer>(depthFilterImage);
}

void Scenario2_GetRawData::OnNavigatedTo(Windows::UI::Xaml::Navigation::NavigationEventArgs^ e)
{
	// Start streaming from the first available source group.
	PickNextMediaSourceAsync();
}

void Scenario2_GetRawData::OnNavigatedFrom(Windows::UI::Xaml::Navigation::NavigationEventArgs^ e)
{
	CleanupMediaCaptureAsync();
}

void Scenario2_GetRawData::NextButton_Click(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e)
{
	PickNextMediaSourceAsync();
}

void Scenario2_GetRawData::captureButton_Click(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e)
{
	captureButtonPressed = 1;
}

task<void> Scenario2_GetRawData::PickNextMediaSourceAsync()
{
	NextButton->IsEnabled = false;
	return PickNextMediaSourceWorkerAsync()
		.then([this]()
	{
		NextButton->IsEnabled = true;
	}, task_continuation_context::use_current());
}

task<void> Scenario2_GetRawData::PickNextMediaSourceWorkerAsync()
{
	return CleanupMediaCaptureAsync().then([this]()
	{
		return create_task(MediaFrameSourceGroup::FindAllAsync());
	}).then([this](IVectorView<MediaFrameSourceGroup^>^ allGroups)
	{
		std::vector<MediaFrameSourceGroup^> eligableGroups;
		for (auto const& group : allGroups)
		{
			auto sourceInfos = group->SourceInfos;

			// Keep this group if it at least supports color, as the other sources must be correlated with the color source.
			if (group != nullptr && std::any_of(begin(sourceInfos), end(sourceInfos),
				[](MediaFrameSourceInfo^ sourceInfo) { return sourceInfo != nullptr && sourceInfo->SourceKind == MediaFrameSourceKind::Color; }))
			{
				eligableGroups.push_back(group);
			}
		}

		if (eligableGroups.size() == 0)
		{
			m_logger->Log("No valid source groups found");
			return task_from_result();
		}

		// Pick next group in the array after each time the Next button is clicked.
		m_selectedSourceGroupIndex = (m_selectedSourceGroupIndex + 1) % eligableGroups.size();

		m_logger->Log("Found " + eligableGroups.size().ToString() + " groups and " +
			"selecting index [" + m_selectedSourceGroupIndex.ToString() + "] : " +
			eligableGroups[m_selectedSourceGroupIndex]->DisplayName);

		MediaFrameSourceGroup^ selectedGroup = eligableGroups[m_selectedSourceGroupIndex];

		// Initialize MediaCapture with selected group.
		return TryInitializeMediaCaptureAsync(selectedGroup)
			.then([this, selectedGroup](bool initialized)
		{
			if (!initialized)
			{
				return CleanupMediaCaptureAsync();
			}

			// Try to find color and depth sources on this source group.
			auto sourceInfos = selectedGroup->SourceInfos;

			auto colorSourceInfo = std::find_if(begin(sourceInfos), end(sourceInfos), [](MediaFrameSourceInfo^ sourceInfo)
			{
				return sourceInfo->SourceKind == MediaFrameSourceKind::Color;
			});

			auto depthSourceInfo = std::find_if(begin(sourceInfos), end(sourceInfos), [](MediaFrameSourceInfo^ sourceInfo)
			{
				return sourceInfo->SourceKind == MediaFrameSourceKind::Depth;
			});

			auto infraredSourceInfo = std::find_if(begin(sourceInfos), end(sourceInfos), [](MediaFrameSourceInfo^ sourceInfo)
			{
				return sourceInfo->SourceKind == MediaFrameSourceKind::Infrared;
			});

			// Reset our frame sources data
			m_frameSources[MediaFrameSourceKind::Color] = FrameSourceState2();
			m_frameSources[MediaFrameSourceKind::Depth] = FrameSourceState2();
			m_frameSources[MediaFrameSourceKind::Infrared] = FrameSourceState2();

			// Store the source info object if a source group was found.
			m_frameSources[MediaFrameSourceKind::Color].sourceInfo = colorSourceInfo != end(sourceInfos) ? *colorSourceInfo : nullptr;
			m_frameSources[MediaFrameSourceKind::Depth].sourceInfo = depthSourceInfo != end(sourceInfos) ? *depthSourceInfo : nullptr;
			m_frameSources[MediaFrameSourceKind::Infrared].sourceInfo = infraredSourceInfo != end(sourceInfos) ? *infraredSourceInfo : nullptr;

			// Enable color always.
			m_frameSources[MediaFrameSourceKind::Color].enabled = true;

			// Enable depth if depth is available.
			m_frameSources[MediaFrameSourceKind::Depth].enabled = m_frameSources[MediaFrameSourceKind::Depth].sourceInfo != nullptr;

			// Enable infrared if infrared is available.
			m_frameSources[MediaFrameSourceKind::Infrared].enabled = m_frameSources[MediaFrameSourceKind::Infrared].sourceInfo != nullptr;

			// Create readers for found sources.
			std::vector<task<void>> createReadersTasks;

			if (m_frameSources[MediaFrameSourceKind::Color].sourceInfo)
			{
				createReadersTasks.push_back(CreateReaderAsync(m_frameSources[MediaFrameSourceKind::Color].sourceInfo));
			}

			if (m_frameSources[MediaFrameSourceKind::Depth].sourceInfo)
			{
				createReadersTasks.push_back(CreateReaderAsync(m_frameSources[MediaFrameSourceKind::Depth].sourceInfo));
			}

			if (m_frameSources[MediaFrameSourceKind::Infrared].sourceInfo)
			{
				createReadersTasks.push_back(CreateReaderAsync(m_frameSources[MediaFrameSourceKind::Infrared].sourceInfo));
			}

			// The when_all method will execute all tasks in parallel, and call the continuation when all tasks have completed.
			// This async method can be called even if no readers are present. In that case the continuation will be called immediately.
			return when_all(begin(createReadersTasks), end(createReadersTasks));
		});
	}, task_continuation_context::get_current_winrt_context());
}

task<void> Scenario2_GetRawData::CreateReaderAsync(MediaFrameSourceInfo^ info)
{
	// Access the initialized frame source by looking up the the Id of the source.
	// Verify that the Id is present, because it may have left the group while were were
	// busy deciding which group to use.
	if (!m_mediaCapture->FrameSources->HasKey(info->Id))
	{
		m_logger->Log("Unable to start " + info->SourceKind.ToString() + " reader: Frame source not found");
		return task_from_result();
	}

	return create_task(m_mediaCapture->CreateFrameReaderAsync(m_mediaCapture->FrameSources->Lookup(info->Id)))
		.then([this, info](MediaFrameReader^ frameReader)
	{
		m_frameSources[info->SourceKind].frameArrivedEventToken = frameReader->FrameArrived +=
			ref new TypedEventHandler<MediaFrameReader^, MediaFrameArrivedEventArgs^>(this, &Scenario2_GetRawData::FrameReader_FrameArrived);

		m_logger->Log(info->SourceKind.ToString() + " reader created");

		// Keep track of created reader and event handler so it can be stopped later.
		m_frameSources[info->SourceKind].reader = frameReader;
		return create_task(frameReader->StartAsync());
	}).then([this, info](MediaFrameReaderStartStatus status)
	{
		if (status != MediaFrameReaderStartStatus::Success)
		{
			m_logger->Log("Unable to start " + info->SourceKind.ToString() + "  reader. Error: " + status.ToString());
		}
	});
}

task<bool> Scenario2_GetRawData::TryInitializeMediaCaptureAsync(MediaFrameSourceGroup^ group)
{
	if (m_mediaCapture != nullptr)
	{
		// Already initialized.
		return task_from_result(true);
	}

	// Initialize mediacapture with the source group.
	m_mediaCapture = ref new MediaCapture();

	auto settings = ref new MediaCaptureInitializationSettings();

	// Select the source we will be reading from.
	settings->SourceGroup = group;

	// This media capture can share streaming with other apps.
	settings->SharingMode = MediaCaptureSharingMode::SharedReadOnly;

	// Only stream video and don't initialize audio capture devices.
	settings->StreamingCaptureMode = StreamingCaptureMode::Video;

	// Set to CPU to ensure frames always contain CPU SoftwareBitmap images,
	// instead of preferring GPU D3DSurface images.
	settings->MemoryPreference = MediaCaptureMemoryPreference::Cpu;

	// Only stream video and don't initialize audio capture devices.
	settings->StreamingCaptureMode = StreamingCaptureMode::Video;

	// Initialize MediaCapture with the specified group.
	// This must occur on the UI thread because some device families
	// (such as Xbox) will prompt the user to grant consent for the
	// app to access cameras.
	// This can raise an exception if the source no longer exists,
	// or if the source could not be initialized.
	return create_task(m_mediaCapture->InitializeAsync(settings))
		.then([this](task<void> initializeMediaCaptureTask)
	{
		try
		{
			// Get the result of the initialization. This call will throw if initialization failed
			// This pattern is documented at https://msdn.microsoft.com/en-us/library/dd997692.aspx
			initializeMediaCaptureTask.get();
			m_logger->Log("MediaCapture is successfully initialized in shared mode.");
			return true;
		}
		catch (Exception^ exception)
		{
			m_logger->Log("Failed to initialize media capture: " + exception->Message);
			return false;
		}
	});
}

task<void> Scenario2_GetRawData::CleanupMediaCaptureAsync()
{
	task<void> cleanupTask = task_from_result();

	if (m_mediaCapture != nullptr)
	{
		for (FrameSourceState2 frameSourceState : values(m_frameSources))
		{
			if (frameSourceState.reader)
			{
				frameSourceState.reader->FrameArrived -= frameSourceState.frameArrivedEventToken;
				cleanupTask = cleanupTask && create_task(frameSourceState.reader->StopAsync());
			}

			frameSourceState.enabled = false;
			frameSourceState.sourceInfo = nullptr;
			frameSourceState.reader = nullptr;
			frameSourceState.latestFrame = nullptr;
		}

		m_mediaCapture = nullptr;
	}
	return cleanupTask;
}

void Scenario2_GetRawData::FrameReader_FrameArrived(MediaFrameReader^ sender, MediaFrameArrivedEventArgs^ args)
{
	// TryAcquireLatestFrame will return the latest frame that has not yet been acquired.
	// This can return null if there is no such frame, or if the reader is not in the
	// "Started" state. The latter can occur if a FrameArrived event was in flight
	// when the reader was stopped.
	if (MediaFrameReference^ candidateFrame = sender->TryAcquireLatestFrame())
	{
		// Since multiple sources will be receiving frames, we must synchronize access to m_frameSources.
		auto lock = m_frameLock.LockExclusive();

		// Buffer frame for later usage.
		m_frameSources[candidateFrame->SourceKind].latestFrame = candidateFrame;

		auto frameSourceObjects = values(m_frameSources);
		bool allFramesBuffered = std::none_of(frameSourceObjects.begin(), frameSourceObjects.end(),
			[](FrameSourceState2 const& frameSourceState)
		{
			return frameSourceState.enabled && frameSourceState.latestFrame == nullptr;
		});

		// If we have frames from currently enabled sources, render to UI.
		if (allFramesBuffered)
		{
			bool colorEnabled = m_frameSources[MediaFrameSourceKind::Color].enabled;
			bool depthEnabled = m_frameSources[MediaFrameSourceKind::Depth].enabled;
			bool infraredEnabled = m_frameSources[MediaFrameSourceKind::Infrared].enabled;

			MediaFrameReference^ colorFrame = m_frameSources[MediaFrameSourceKind::Color].latestFrame;
			MediaFrameReference^ depthFrame = m_frameSources[MediaFrameSourceKind::Depth].latestFrame;
			MediaFrameReference^ infraredFrame = m_frameSources[MediaFrameSourceKind::Infrared].latestFrame;

			if (colorEnabled)
			{
				m_colorFrameRenderer->ProcessColorFrame(colorFrame);
			}
			if (depthEnabled)
			{
				m_depthFrameRenderer->ProcessDepthFrame(depthFrame);
			}
			if (infraredEnabled)
			{
				m_infraredFrameRenderer->ProcessInfraredFrame(infraredFrame);
			}

			if (captureButtonPressed)
			{
				m_logger->Log("Capturing Frame");

				if (colorEnabled)
				{
					m_singleColorFrameRenderer->ProcessColorFrame(colorFrame);
				}
				if (depthEnabled)
				{
					m_singleDepthFrameRenderer->ProcessDepthFrame(depthFrame);
				}
				if (infraredEnabled)
				{
					m_singleInfraredFrameRenderer->ProcessInfraredFrame(infraredFrame);
				}
				if (colorEnabled && depthEnabled)
				{
					m_depthFilterFrameRenderer->ProcessDepthAndColorFrames(colorFrame, depthFrame);
				}

				captureButtonPressed = 0;
			}

			// clear buffered frames if used
			if (colorEnabled)
			{
				m_frameSources[MediaFrameSourceKind::Color].latestFrame = nullptr;
			}
			if (depthEnabled)
			{
				m_frameSources[MediaFrameSourceKind::Depth].latestFrame = nullptr;
			}
			if (infraredEnabled)
			{
				m_frameSources[MediaFrameSourceKind::Infrared].latestFrame = nullptr;
			}
		}
	}
	else
	{
		m_logger->Log("Unable to acquire frame");
	}
}
