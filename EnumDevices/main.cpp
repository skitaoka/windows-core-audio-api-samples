#include <memory>
#include <sstream>

#include <wrl.h>
#include <propvarutil.h>
#include <mmdeviceapi.h>
#include <functiondiscoverykeys_devpkey.h>

namespace {

using prop_string_t = std::unique_ptr<WCHAR, decltype(&CoTaskMemFree)>;

#define prop_string_null prop_string_t(nullptr, CoTaskMemFree)

prop_string_t GetDeviceId(IN Microsoft::WRL::ComPtr<IMMDevice> const& device) {
  LPWSTR buffer = nullptr;
  if SUCCEEDED(device->GetId(&buffer)) {
    return prop_string_t(buffer, CoTaskMemFree);
  }
  return prop_string_null;
}

std::wstring GetDeviceState(IN Microsoft::WRL::ComPtr<IMMDevice> const& device) {
  DWORD state = 0;
  if SUCCEEDED(device->GetState(&state)) {
    switch (state) {
    case DEVICE_STATE_ACTIVE: return L"ACTIVE";
    case DEVICE_STATE_DISABLED: return L"DISABLED";
    case DEVICE_STATE_NOTPRESENT: return L"NOTPRESENT";
    case DEVICE_STATE_UNPLUGGED: return L"UNPLUGGED";
    }
  }
  return L"UNKNOWN";
}

prop_string_t GetPropString(IN Microsoft::WRL::ComPtr<IPropertyStore> const& prop,
                            IN PROPERTYKEY const& key) {
  PROPVARIANT var;
  ::PropVariantInit(&var);
  if SUCCEEDED(prop->GetValue(key, &var)) {
    LPWSTR str;
    if SUCCEEDED(::PropVariantToStringAlloc(var, &str)) {
      ::PropVariantClear(&var);
      return prop_string_t(str, CoTaskMemFree);
    }
  }
  ::PropVariantClear(&var);
  return prop_string_null;
}

} // end of anonymous namespace

int main() {
  if SUCCEEDED(::CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED)) {
    {
      Microsoft::WRL::ComPtr<IMMDeviceEnumerator> enumerator;
      if SUCCEEDED(::CoCreateInstance(__uuidof(MMDeviceEnumerator),
                                      nullptr,
                                      CLSCTX_INPROC_SERVER,
                                      IID_PPV_ARGS(enumerator.GetAddressOf()))) {

        // 出力先デバイスを列挙
        Microsoft::WRL::ComPtr<IMMDeviceCollection> devices;
        if SUCCEEDED(enumerator->EnumAudioEndpoints(EDataFlow::eAll, // eRender, eCapture, eAll
                                                    DEVICE_STATEMASK_ALL,
                                                    // DEVICE_STATE_ACTIVE: 有効化されている
                                                    // DEVICE_STATE_DISABLED: 無効化されている
                                                    // DEVICE_STATE_NOTPRESENT: システムに無登録
                                                    // DEVICE_STATE_UNPLUGGED: 物理的に接続されていない
                                                    devices.GetAddressOf())) {

          std::wostringstream out;

          // デバイス数を取得
          UINT uNumDevices;
          if SUCCEEDED(devices->GetCount(&uNumDevices)) {
            for (UINT i = 0; i < uNumDevices; ++i) {
              // デバイスを取得
              Microsoft::WRL::ComPtr<IMMDevice> device;
              if SUCCEEDED(devices->Item(i, device.GetAddressOf())) {
                // デバイスの情報を取得
                auto const id = GetDeviceId(device); // デバイスID
                auto const state = GetDeviceState(device); // デバイスの状態

                Microsoft::WRL::ComPtr<IPropertyStore> prop;
                if SUCCEEDED(device->OpenPropertyStore(STGM_READ, prop.GetAddressOf())) {
                  auto const name = GetPropString(prop, PKEY_Device_FriendlyName); // フルネーム
                  auto const desc = GetPropString(prop, PKEY_Device_DeviceDesc); // ショートネーム
                  auto const audioif = GetPropString(prop, PKEY_DeviceInterface_FriendlyName); // 物理デバイス名
                  out << L"[" << i << L"] id=" << id.get() << L", state=" << state << L", name=" << name.get()
                    << L", desc=" << desc.get() << L", audioif=" << audioif.get() << L'\n';
                }
              }
            }
          }

          std::wstring const names = out.str();
          ::MessageBoxW(HWND_DESKTOP, names.c_str(), L"devices", MB_OK);
        }
      }
    }
    ::CoUninitialize();
  }

  return 0;
}