#include <memory>
#include <sstream>

#include <wrl.h>
#include <propvarutil.h>
#include <mmdeviceapi.h>
#include <functiondiscoverykeys_devpkey.h>

using prop_string_t = std::unique_ptr<WCHAR, decltype(&CoTaskMemFree)>;

prop_string_t PropertyStoreReadStringValue(
  IN Microsoft::WRL::ComPtr<IPropertyStore> prop, IN PROPERTYKEY const& key) {
  PROPVARIANT var;
  ::PropVariantInit(&var);
  if SUCCEEDED(prop->GetValue(key, &var)) {
    LPWSTR str;
    if SUCCEEDED(::PropVariantToStringAlloc(var, &str)) {
      ::PropVariantClear(&var);

      return prop_string_t(str, CoTaskMemFree);
    }
  }

  return prop_string_t(nullptr, CoTaskMemFree);
}

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
              // 各デバイスの情報を取得
              Microsoft::WRL::ComPtr<IMMDevice> device;
              if SUCCEEDED(devices->Item(i, device.GetAddressOf())) {
                Microsoft::WRL::ComPtr<IPropertyStore> prop;
                if SUCCEEDED(device->OpenPropertyStore(STGM_READ, prop.GetAddressOf())) {
                  auto description = PropertyStoreReadStringValue(prop, PKEY_Device_DeviceDesc);
                  out << L"[" << i << L"] " << description.get() << L'\n';
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