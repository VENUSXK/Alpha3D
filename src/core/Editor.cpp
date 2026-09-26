#include "core/Editor.h"


#include <algorithm>
#include <cmath>
#include <cfloat>
#include <cstdint>
#include <cstdio>
#include <string>
#include <vector>

#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>
#include <imgui_internal.h>

#include "core/Log.h"
#include "core/viewport.h"
#include "core/Window.h"

#include "renderer/IBL.h"
#include "renderer/Camera.h"
#include "utils/RenderProfiler.h"

#include "scene/GameObject.h"
#include "scene/Scene.h"

#include "components/SkyAtmosphere.h"
#include "components/VolumetricCloud.h"

#include "utils/Transform.h"

void Editor::Init(Window* window)
{
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();

    ImGuiIO& io = ImGui::GetIO();
	io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;

	currentScale = window->GetCurrentScale();

	this->font_name = "assets/fonts/Inter-Medium.ttf";
	this->font_small = io.Fonts->AddFontFromFileTTF(this->font_name.c_str(), 13.f * currentScale);
    // io.Fonts->AddFontFromFileTTF("assets/fonts/Inter-Medium.ttf", 14.0f * xscale);

    ImGui::GetStyle().ScaleAllSizes(currentScale);
    LOG_INFO("editor created x{} scale", currentScale);
    ImGui_ImplGlfw_InitForOpenGL(window->GetGLFWWindow(), true);
    ImGui_ImplOpenGL3_Init();
}

void Editor::Update(const Window& window)
{
	const float newScale = window.GetCurrentScale();
	if (newScale <= 0.0f || std::abs(newScale - currentScale) < 0.001f) {
		return;
	}

	const float scaleRatio = newScale / currentScale;
	ImGui::GetStyle().ScaleAllSizes(scaleRatio);

	ImGuiIO& io = ImGui::GetIO();
	io.Fonts->Clear();
	this->font_small = io.Fonts->AddFontFromFileTTF(this->font_name.c_str(), 13.f * newScale);
	io.FontDefault = font_small;

	currentScale = newScale;
	LOG_INFO("Editor UI scale changed to {}", currentScale);
}

Editor::~Editor()
{
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
}


void Editor::ShowToast(const std::string& message, float duration) {
	toast_message = message;
	toast_timer = duration;
	toast_duration = duration;
}

void Editor::BeginFrame(Viewport& viewport)
{
    ImGui_ImplOpenGL3_NewFrame();
    
	ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();
	ImGui::DockSpaceOverViewport();


	// 1.1 视窗
	ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0)); // style

	ImGui::Begin("Viewport");

	ImVec2 viewportSize = ImGui::GetContentRegionAvail();
	viewport.Resize(viewportSize.x, viewportSize.y);

	ImGui::Image((void*)(intptr_t)viewport.GetColorTexture(), ImGui::GetContentRegionAvail(), ImVec2(0, 1), ImVec2(1, 0));
	this->isViewportHovered = ImGui::IsItemHovered();


	ImGui::End();

	ImGui::PopStyleVar(); // style
}

void Editor::BeginCamera(Camera & camera) {
	ImGui::Begin("Camera");

	ImGui::Text("View:");

	auto fovToMm = [](float fov_deg, float sensor_diag = 43.27f) -> float {
		float fov_rad = glm::radians(fov_deg);
		return (sensor_diag / 2.0f) / std::tan(fov_rad / 2.0f);
	};

	auto mmToFov = [](float mm, float sensor_diag = 43.27f) -> float {
		return glm::degrees(2.0f * std::atan((sensor_diag / 2.0f) / mm));
	};

	float fov = camera.GetFov();
	float mm = fovToMm(fov);

	if (ImGui::SliderFloat("Focal Length", &mm, fovToMm(170.0f), fovToMm(10.0f), "%.1f mm")) {
		mm = glm::clamp(mm, 1.0f, 500.0f); // 防止极端值
		camera.SetFov(mmToFov(mm));
	}

	ImGui::Text("Movement:");

	glm::vec3 position = camera.GetPosition();
	if (ImGui::InputFloat3("Position", &position.x, "%.2f")) {
		camera.SetPosition(position);
	}

	glm::vec3 direction = camera.GetDirection();
	if (ImGui::InputFloat3("Direction", &direction.x, "%.3f")) {
		camera.SetDirection(direction);
	}
	ImGui::InputFloat("Speed", &camera.moveSpeed, 0.5f, 1.0f, "%.1f");


	ImGui::End();
}

void Editor::BeginDetails(GameObject& game_object) {
	Transform& transform = game_object.GetTransform();

	if (!ImGui::Begin("Details")) {
		ImGui::End();
		return;
	}

	ImGui::Text(game_object.GetName().c_str());

	if (ImGui::CollapsingHeader("Transform", ImGuiTreeNodeFlags_DefaultOpen)) {
		ImGui::Text("Location:");
		glm::vec3 position = transform.position;
		bool positionChanged = false;
		positionChanged |= ImGui::DragFloat("X##trans_x", &position[0], 0.005f, -FLT_MAX, +FLT_MAX, "%.2f m");
		positionChanged |= ImGui::DragFloat("Y##trans_y", &position[1], 0.005f, -FLT_MAX, +FLT_MAX, "%.2f m");
		positionChanged |= ImGui::DragFloat("Z##trans_z", &position[2], 0.005f, -FLT_MAX, +FLT_MAX, "%.2f m");
		if (positionChanged)
			transform.SetPosition(position);
		ImGui::Spacing();

		ImGui::Text("Rotation:");
		glm::vec3 rotation = transform.rotation;
		bool rotationChanged = false;
		rotationChanged |= ImGui::DragFloat("X##rotate_x", &rotation[0], 0.2f, -FLT_MAX, +FLT_MAX, "%.0f deg");
		rotationChanged |= ImGui::DragFloat("Y##rotate_y", &rotation[1], 0.2f, -FLT_MAX, +FLT_MAX, "%.0f deg");
		rotationChanged |= ImGui::DragFloat("Z##rotate_z", &rotation[2], 0.2f, -FLT_MAX, +FLT_MAX, "%.0f deg");
		if (rotationChanged)
			transform.SetRotation(rotation);

		ImGui::Text("Scale:");
		glm::vec3 scale = transform.scale;
		if (ImGui::DragFloat3("Scale##scale", &scale.x, 0.05f, -FLT_MAX, +FLT_MAX, "%.3f"))
			transform.SetScale(scale);
	}


	if (ImGui::CollapsingHeader("Animate", ImGuiTreeNodeFlags_DefaultOpen)) {
		ImGui::Text("Rotation Speed:");
		ImGui::SliderFloat("X##rotate_speed_x", &game_object.rotate_speed_x, -90.0f, 90.0f, "%.0f deg/s");
		ImGui::SliderFloat("Y##rotate_speed_y", &game_object.rotate_speed_y, -90.0f, 90.0f, "%.0f deg/s");
		ImGui::SliderFloat("Z##rotate_speed_z", &game_object.rotate_speed_z, -90.0f, 90.0f, "%.0f deg/s");
	}

	if (ImGui::CollapsingHeader("Inspector", ImGuiTreeNodeFlags_DefaultOpen)) {

		if (game_object.light) {
			ImGui::Text("Light:");
			static float light_intensity_low = 0.0f, light_intensity_high = 20.0f;
			ImGui::ColorEdit3("Color##1", (float*)&(*game_object.light).color, ImGuiColorEditFlags_DisplayRGB | ImGuiColorEditFlags_Float);
			ImGui::SliderScalar("Intensity", ImGuiDataType_Float, &(*game_object.light).intensity, &light_intensity_low, &light_intensity_high, "%.1f lm");
		}

		if (game_object.pbr_sphere) {
			ImGui::Text("PBR Parameters:");
			static float pbr_roughness_low = 0.01f, pbr_roughness_high = 1.0f;
			static float pbr_metallic_low = 0.0f, pbr_metallic_high = 1.0f;
			ImGui::ColorEdit3("Albedo##pbr_albedo", (float*)&(*game_object.pbr_sphere).albedo, ImGuiColorEditFlags_DisplayRGB | ImGuiColorEditFlags_Float);
			ImGui::SliderScalar("Roughness", ImGuiDataType_Float, &(*game_object.pbr_sphere).roughness, &pbr_roughness_low, &pbr_roughness_high, "%.2f");
			ImGui::SliderScalar("Metallic", ImGuiDataType_Float, &(*game_object.pbr_sphere).metallic, &pbr_metallic_low, &pbr_metallic_high, "%.2f");
		}

		if (!game_object.light && !game_object.pbr_sphere) {
			ImGui::Text("This object has no specific properties.");
		}

	}

	ImGui::End();

}


void Editor::EndFrame()
{
	if (toast_timer > 0.0f) {
		toast_timer -= ImGui::GetIO().DeltaTime;
		if (toast_timer < 0.0f) {
			toast_timer = 0.0f;
		}

		float alpha = toast_timer < toast_fade_duration ? toast_timer / toast_fade_duration : 1.0f;
		alpha = alpha < 0.0f ? 0.0f : (alpha > 1.0f ? 1.0f : alpha);

		ImGuiViewport* viewport = ImGui::GetMainViewport();
		ImVec2 work_pos = viewport->WorkPos;

		ImVec2 window_pos(
			work_pos.x + 20.0f,
			work_pos.y + 40.0f
		);

		ImGui::SetNextWindowPos(window_pos, ImGuiCond_Always, ImVec2(0.0f, 0.0f));
		ImGui::SetNextWindowBgAlpha(0.85f * alpha);

		ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);

		ImGuiWindowFlags flags =
			ImGuiWindowFlags_NoDecoration |
			ImGuiWindowFlags_AlwaysAutoResize |
			ImGuiWindowFlags_NoSavedSettings |
			ImGuiWindowFlags_NoFocusOnAppearing |
			ImGuiWindowFlags_NoNav |
			ImGuiWindowFlags_NoMove |
			ImGuiWindowFlags_NoInputs;

		if (ImGui::Begin("PhotoToast", nullptr, flags)) {
			ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 1.0f, 1.0f, alpha));
			ImGui::TextUnformatted(toast_message.c_str());
			ImGui::PopStyleColor();
		}
		ImGui::End();
		ImGui::PopStyleVar();

		if (toast_timer < 0.0f) {
			toast_timer = 0.0f;
		}
	}

	ImGui::Render();
	ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

void Editor::BeginPerformance(RenderProfiler& profiler)
{
	ImGui::Begin("Performance");

	bool enabled = profiler.IsEnabled();
	if (ImGui::Checkbox("Profiling", &enabled)) profiler.SetEnabled(enabled);
	ImGui::SameLine();
	bool paused = profiler.IsPaused();
	if (ImGui::Checkbox("Pause", &paused)) profiler.SetPaused(paused);
	ImGui::SameLine();
	if (ImGui::Button("Reset")) profiler.Reset();

	ImGui::SameLine();

	const double averageFrameMs = profiler.GetAverageFrameCpuMs();
	const double averageFps = averageFrameMs > 0.0 ? 1000.0 / averageFrameMs : 0.0;
	ImGui::Text("Frame Avg: %.2f ms  |  FPS Avg: %.1f", averageFrameMs, averageFps);

	auto stats = profiler.GetStats();
	std::stable_sort(stats.begin(), stats.end(), [](const auto& left, const auto& right) {
		if (left.averageGpuMs != right.averageGpuMs)
			return left.averageGpuMs > right.averageGpuMs;
		return left.averageCpuMs > right.averageCpuMs;
	});
	const double averageFrameGpuMs = profiler.GetAverageFrameGpuMs();

	const ImGuiTableFlags flags = ImGuiTableFlags_BordersInnerV |
		ImGuiTableFlags_RowBg | ImGuiTableFlags_Resizable |
		ImGuiTableFlags_ScrollY | ImGuiTableFlags_SizingStretchProp;
	ImGui::TextUnformatted("Frame Regions");
	if (ImGui::BeginTable("Frame regions", 5, flags, ImVec2(0.0f, 260.0f))) {
		ImGui::TableSetupColumn("Region", ImGuiTableColumnFlags_WidthStretch, 2.5f);
		ImGui::TableSetupColumn("CPU", ImGuiTableColumnFlags_WidthFixed, 75.0f);
		ImGui::TableSetupColumn("GPU", ImGuiTableColumnFlags_WidthFixed, 75.0f);
		ImGui::TableSetupColumn("Avg GPU", ImGuiTableColumnFlags_WidthFixed, 75.0f);
		ImGui::TableSetupColumn("GPU Avg Load", ImGuiTableColumnFlags_WidthStretch, 1.0f);
		ImGui::TableHeadersRow();

		for (const auto& region : stats) {
			if (region.name.rfind("One-time/", 0) == 0) continue;
			ImGui::PushID(region.name.c_str());
			ImGui::TableNextRow();
			ImGui::TableSetColumnIndex(0);
			ImGui::TextUnformatted(region.name.c_str());
			if (ImGui::IsItemHovered()) {
				ImGui::SetTooltip("GPU min %.3f ms, max %.3f ms\n%llu CPU / %llu GPU samples",
					region.minGpuMs, region.maxGpuMs,
					static_cast<unsigned long long>(region.cpuSamples),
					static_cast<unsigned long long>(region.gpuSamples));
			}
			ImGui::TableSetColumnIndex(1);
			if (region.name == "Top-level GPU") ImGui::TextUnformatted("-");
			else ImGui::Text("%.3f ms", region.cpuMs);
			ImGui::TableSetColumnIndex(2);
			if (region.gpuSamples == 0 && region.gpuPending) ImGui::TextUnformatted("pending");
			else ImGui::Text("%.3f ms", region.gpuMs);
			ImGui::TableSetColumnIndex(3);
			ImGui::Text("%.3f ms", region.averageGpuMs);
			ImGui::TableSetColumnIndex(4);
			const float fraction = averageFrameGpuMs > 0.0
				? static_cast<float>(std::min(region.averageGpuMs / averageFrameGpuMs, 1.0)) : 0.0f;
			char loadLabel[16];
			snprintf(loadLabel, sizeof(loadLabel), "%.1f%%", fraction * 100.0f);
			ImGui::ProgressBar(fraction, ImVec2(-FLT_MIN, 0.0f), loadLabel);
			ImGui::PopID();
		}
		ImGui::EndTable();
	}

	bool hasOneTimeTasks = false;
	for (const auto& region : stats) {
		if (region.name.rfind("One-time/", 0) == 0) {
			hasOneTimeTasks = true;
			break;
		}
	}

	if (hasOneTimeTasks) {
		ImGui::Spacing();
		ImGui::TextUnformatted("One-time Tasks");
		if (ImGui::BeginTable("One-time tasks", 4,
			ImGuiTableFlags_BordersInnerV | ImGuiTableFlags_RowBg |
			ImGuiTableFlags_Resizable | ImGuiTableFlags_SizingStretchProp)) {
			ImGui::TableSetupColumn("Task", ImGuiTableColumnFlags_WidthStretch, 2.5f);
			ImGui::TableSetupColumn("CPU", ImGuiTableColumnFlags_WidthFixed, 85.0f);
			ImGui::TableSetupColumn("GPU", ImGuiTableColumnFlags_WidthFixed, 85.0f);
			ImGui::TableSetupColumn("Avg GPU", ImGuiTableColumnFlags_WidthFixed, 85.0f);
			ImGui::TableHeadersRow();

			for (const auto& task : stats) {
				constexpr const char* prefix = "One-time/";
				if (task.name.rfind(prefix, 0) != 0) continue;
				ImGui::PushID(task.name.c_str());
				ImGui::TableNextRow();
				ImGui::TableSetColumnIndex(0);
				ImGui::TextUnformatted(task.name.c_str() + std::char_traits<char>::length(prefix));
				if (ImGui::IsItemHovered()) {
					ImGui::SetTooltip("GPU min %.3f ms, max %.3f ms\n%llu samples",
						task.minGpuMs, task.maxGpuMs,
						static_cast<unsigned long long>(task.gpuSamples));
				}
				ImGui::TableSetColumnIndex(1);
				ImGui::Text("%.3f ms", task.cpuMs);
				ImGui::TableSetColumnIndex(2);
				if (task.gpuSamples == 0 && task.gpuPending) ImGui::TextUnformatted("pending");
				else ImGui::Text("%.3f ms", task.gpuMs);
				ImGui::TableSetColumnIndex(3);
				ImGui::Text("%.3f ms", task.averageGpuMs);
				ImGui::PopID();
			}
			ImGui::EndTable();
		}
	}

	ImGui::End();
}

bool Editor::HasPerformanceAffectingEdit() const
{
	const ImGuiContext* context = ImGui::GetCurrentContext();
	return context != nullptr &&
		(context->ActiveIdHasBeenEditedThisFrame ||
			(context->DeactivatedItemData.ElapseFrame == context->FrameCount &&
			 context->DeactivatedItemData.HasBeenEditedBefore));
}


bool Editor::Hover() const
{
    return ImGui::GetIO().WantCaptureMouse;
}

bool Editor::WantCaptureKeyboard() const
{
    return ImGui::GetIO().WantCaptureKeyboard;
}


void Editor::BeginEnvironment(IBL& ibl) {

	ImGui::Begin("Environment");

	ImGui::Text("Cube Maps:");

	const auto& names = ibl.GetNames();

	// 把 vector<string> 转成 ImGui 需要的格式
	std::vector<const char*> items;
	for (const auto& n : names) items.push_back(n.c_str());

	int selected = ibl.GetSelected();
	if (ImGui::Combo("HDRI", &selected, items.data(), (int)items.size()))
		ibl.Select(selected);

	ImGui::End();
}

void Editor::BeginSkyAtmosphere(SkyAtmosphere& sky) {
	SkyAtmosphereParams& p = sky.parameters;
	bool parametersChanged = false;
	ImGui::Begin("Sky Atmosphere");

	if (ImGui::CollapsingHeader("Sun", ImGuiTreeNodeFlags_DefaultOpen)) {
		parametersChanged |= ImGui::DragFloat("Longitude", &p.sunLongitude, 0.5f, -180.0f, 180.0f, "%.1f deg");
		parametersChanged |= ImGui::DragFloat("Latitude", &p.sunLatitude, 0.5f, -90.0f, 90.0f, "%.1f deg");
		p.sunLatitude = glm::clamp(p.sunLatitude, -90.0f, 90.0f);
		if (parametersChanged) {
			const float longitude = glm::radians(p.sunLongitude);
			const float latitude = glm::radians(p.sunLatitude);
			p.lightDirection = glm::normalize(glm::vec3(
				cos(latitude) * cos(longitude),
				sin(latitude),
				cos(latitude) * sin(longitude)));
		}
		parametersChanged |= ImGui::SliderFloat("Light Intensity", &p.lightIntensity, 0.0f, 100.0f, "%.1f");
	}

	if (ImGui::CollapsingHeader("Planet", ImGuiTreeNodeFlags_DefaultOpen)) {
		parametersChanged |= ImGui::DragFloat("Camera Height", &p.cameraHeight, 100.0f, 1.0f, 1000000.0f, "%.0f m");
		parametersChanged |= ImGui::DragFloat("Planet Radius", &p.planetRadius, 1000.0f, 1.0f, FLT_MAX, "%.0f m");
		parametersChanged |= ImGui::DragFloat("Atmosphere Radius", &p.atmosphereRadius, 1000.0f, p.planetRadius + 1.0f, FLT_MAX, "%.0f m");
		p.atmosphereRadius = glm::max(p.atmosphereRadius, p.planetRadius + 1.0f);
	}

	if (ImGui::CollapsingHeader("Scattering", ImGuiTreeNodeFlags_DefaultOpen)) {
		parametersChanged |= ImGui::Checkbox("Use Rayleigh Scattering", &p.useRayleigh);
		parametersChanged |= ImGui::Checkbox("Use Mie Scattering", &p.useMie);
		parametersChanged |= ImGui::Checkbox("Use Absorption", &p.useAbsorption);
	}

	if (ImGui::CollapsingHeader("Ray Marching", ImGuiTreeNodeFlags_DefaultOpen)) {
		parametersChanged |= ImGui::SliderInt("Primary Steps", &p.primarySteps, 1, 128);
		parametersChanged |= ImGui::SliderInt("Light Steps", &p.lightSteps, 1, 128);
	}
	
	if (ImGui::CollapsingHeader("Acceleration", ImGuiTreeNodeFlags_DefaultOpen)) {
		parametersChanged |= ImGui::Checkbox("Use Transmittance LUT", &p.useTransmittanceLUT);
		parametersChanged |= ImGui::Checkbox("Use Sky-View LUT", &p.useSkyViewLUT);
	}

	if (parametersChanged)
		sky.MarkParametersDirty();

	ImGui::End();
}

void Editor::BeginVolumetricCloud(VolumetricCloud& cloud) {
	VolumetricCloudParameters& p = cloud.parameters;

	bool parametersChanged = false;
	if (!ImGui::Begin("Volumetric Cloud")) {
		ImGui::End();
		return;
	}

	if (ImGui::CollapsingHeader("Density", ImGuiTreeNodeFlags_DefaultOpen)) {
		parametersChanged |= ImGui::SliderFloat("Density Scale", &p.densityScale, 0.0f, 10.0f, "%.2f");
		parametersChanged |= ImGui::SliderFloat("Extinction", &p.extinction, 0.0f, 10.0f, "%.2f");
	}

	if (ImGui::CollapsingHeader("Cloud Map", ImGuiTreeNodeFlags_DefaultOpen)) {
		parametersChanged |= ImGui::DragFloat("Cloud Map Scale", &p.cloudMapScale, 0.5f, 1.0f, 10000000.0f, "%.2f");
	}

	if (ImGui::CollapsingHeader("Noise", ImGuiTreeNodeFlags_DefaultOpen)) {
		parametersChanged |= ImGui::Checkbox("Use Low Frequency Noise", &p.useLowFreqNoise);
		parametersChanged |= ImGui::DragFloat("Low Frequency Noise Scale", &p.lowFreqNoiseScale, 0.5f, 1.0f, 10000000.0f, "%.2f");

		parametersChanged |= ImGui::Checkbox("Use High Frequency Noise", &p.useHighFreqNoise);
		parametersChanged |= ImGui::DragFloat("High Frequency Noise Scale", &p.highFreqNoiseScale, 0.5f, 1.0f, 10000000.0f, "%.2f");

		parametersChanged |= ImGui::SliderFloat("Erosion Strength", &p.erosionStrength, 0.0f, 1.0f, "%.2f");
	}

	if (ImGui::CollapsingHeader("Lighting", ImGuiTreeNodeFlags_DefaultOpen)) {
		parametersChanged |= ImGui::DragFloat3("Light Direction", &p.lightDirection.x, 0.01f, -1.0f, 1.0f, "%.2f");
		parametersChanged |= ImGui::ColorEdit3("Light Color", &p.lightColor.x, ImGuiColorEditFlags_Float | ImGuiColorEditFlags_HDR);
		parametersChanged |= ImGui::ColorEdit3("Ambient Light", &p.ambientLight.x, ImGuiColorEditFlags_Float | ImGuiColorEditFlags_HDR);
		parametersChanged |= ImGui::SliderFloat("Light Intensity", &p.lightIntensity, 0.0f, 50.0f, "%.1f");
	}

	if (ImGui::CollapsingHeader("Ray Marching", ImGuiTreeNodeFlags_DefaultOpen)) {
		parametersChanged |= ImGui::SliderInt("Primary Steps", &p.primarySteps, 1, 96);
		parametersChanged |= ImGui::SliderInt("Light Steps", &p.lightSteps, 1, 64);
		parametersChanged |= ImGui::SliderFloat("Ray Jitter", &p.rayJitterStrength, 0.0f, 1.0f, "%.2f");
		parametersChanged |= ImGui::DragFloat("Transmittance Cutoff", &p.transmittanceCutoff, 0.0005f, 0.0001f, 0.1f, "%.4f");
	}

	if (ImGui::CollapsingHeader("Nubis Lighting", ImGuiTreeNodeFlags_DefaultOpen)) {
		parametersChanged |= ImGui::SliderFloat("Phase G", &p.phaseG, -0.95f, 0.95f, "%.2f");
	}

	if (ImGui::CollapsingHeader("Cloud Volume", ImGuiTreeNodeFlags_DefaultOpen)) {
		glm::vec3 scale = p.cloudMapVolumeScale;
		glm::vec3 translation = p.cloudMapVolumeTranslation;
		bool transformChanged = false;
		transformChanged |= ImGui::DragFloat3("Volume Scale", &scale.x, 0.5f, 0.01f, 10000000.0f, "%.4f");
		transformChanged |= ImGui::DragFloat3("Volume Translation", &translation.x, 0.5f, -1000.0f, 10000000.0f, "%.4f");
		if (transformChanged) {
			scale = glm::max(scale, glm::vec3(0.01f));
			cloud.updateVolumeTransform(scale, translation);
			parametersChanged = true;
			p.cloudMapVolumeScale = scale;
			p.cloudMapVolumeTranslation = translation;
		}
	}

	if (parametersChanged)
		cloud.MarkParametersDirty();

	ImGui::End();
}

void Editor::BeginHierarchy(Scene& scene) {
	if (!ImGui::Begin("Scene")) {
		ImGui::End();
		return;
	}

	GameObject* selected = scene.GetSelected();
	for (const auto& object : scene.GetGameObjects()) {
		ImGui::PushID(object->GetID());
		if (ImGui::Selectable(object->GetName().c_str(), object.get() == selected)) scene.SetSelected(object->GetID());
		ImGui::PopID();
	}

	ImGui::End();
}
