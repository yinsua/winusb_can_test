//
// Created by yinsua on 2023/7/10.
//

#include "imgui.h"
#include <cstdio>
#include <thread>
#include <format>
#include <map>
#include "api/candle.h"

candle_list_handle clist;
candle_handle hdev;
candle_frame_t frame;
candle_devstate_t state;
uint8_t num_devices;

std::jthread *thread = nullptr;

int scan() {
  if (!candle_list_scan(&clist)) {
    printf("cannot scan for candle devices.\n");
    candle_list_free(clist);
    return -1;
  }

  candle_list_length(clist, &num_devices);

  if (num_devices == 0) {
    printf("cannot find any candle devices.\n");
    return 0;
  }

  printf("detected %d candle device(s):\n", num_devices);

  return 0;
}

int open(int index, int channel = 0, int baud = 1000'000) {
  if (candle_dev_get(clist, index, &hdev)) {
    candle_dev_get_state(hdev, &state);

    uint8_t channels;
    candle_channel_count(hdev, &channels);
    printf("%d: state=%d interfaces=%d path=%s\n", index, state, channels, candle_dev_get_path(hdev));

    candle_dev_free(hdev);
  } else {
    printf("error getting info for device %d\n", index);
  }

  if (!candle_dev_get(clist, 0, &hdev)) {
    printf("error getting info for device %d\n", 0);
    return -2;
  }
  if (!candle_dev_open(hdev)) {
    printf("could not open candle device (%d)\n", candle_dev_last_error(hdev));
    return -3;
  }

  if (!candle_channel_set_bitrate(hdev, 0, baud)) {
    printf("could not set bitrate.\n");
    return -4;
  }

  if (!candle_channel_start(hdev, channel, 0)) {
    printf("could not start device.\n");
    return -5;
  }
  return 0;
}

void thread_func() {
  if (candle_frame_read(hdev, &frame, 1000)) {
    if (candle_frame_type(&frame) == CANDLE_FRAMETYPE_RECEIVE) {
      uint8_t dlc = candle_frame_dlc(&frame);
      uint8_t *data = candle_frame_data(&frame);
      printf(
          "%10d ID 0x%08x [%d]",
          candle_frame_timestamp_us(&frame),
          candle_frame_id(&frame),
          dlc
      );
      for (int i = 0; i < dlc; i++) {
        printf(" %02X", data[i]);
      }
      printf("\n");
      frame.can_id += 1;
      candle_frame_send(hdev, 0, &frame);
    }
  } else {
    candle_err_t err = candle_dev_last_error(hdev);
    if (err == CANDLE_ERR_READ_TIMEOUT) {
//      printf("Timeout waiting for CAN data\n");
    } else {
      printf("Error reading candle frame: %d\n", err);
      return;
    }
  }
}

void thread_release() {
  if (thread != nullptr) {
    thread->request_stop();
    thread->join();
    free(thread);
    thread = nullptr;
  }
}

void mainwindows() {
  ImGui::Begin("main");
  ImGui::PushItemWidth(100);
  static int index_selected = 0;
  static std::map<int, std::string> devices;
  static bool opened = false;
  if (ImGui::BeginCombo("", num_devices ? devices[index_selected].c_str() : "")) {
    for (int i = 0; i < devices.size(); i++) {
      const bool is_selected = (index_selected == i);
      if (ImGui::Selectable(std::format("{} - {}", i, devices[i]).c_str(), is_selected)) {
        index_selected = i;
      }
      if (is_selected) ImGui::SetItemDefaultFocus();
    }
    ImGui::EndCombo();
  }
  ImGui::SameLine();
  if (ImGui::Button("scan")) {
    scan();
    for (int i = 0; i < num_devices; i++) {
      devices[i] = std::to_string(i);
    }
  }
  ImGui::SameLine();
  if (ImGui::Button("Open")) {
    auto ret = open(index_selected);
    if (ret == 0)
      opened = true;
    thread_release();
    thread = new std::jthread([](std::stop_token st) {
      while (!st.stop_requested()) {
        thread_func();
      }
    });
  }
  ImGui::SameLine();
  if (ImGui::Button("Close")) {
    thread_release();
    candle_channel_stop(hdev, 1);
    candle_dev_close(hdev);
    devices.clear();
    opened = false;
  }

  if (opened) {
    if (ImGui::Button("Send Test")) {
      decltype(frame) f;
      f.can_id = 0x100;
      f.can_dlc = 8;
      for (int i = 0; i < f.can_dlc; i++)
        f.data[i] = i;
      candle_frame_send(hdev, 0, &f);
    }
  }

  ImGui::PopItemWidth();
  ImGui::End();
}
