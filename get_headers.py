import urllib.request
def fetch(url):
    try:
        with urllib.request.urlopen(url) as response:
            return response.read().decode()
    except Exception as e:
        return str(e)
print("--- sdk/os/file.h ---")
print(fetch("https://raw.githubusercontent.com/ClasspadDev/hollyhock-3/main/sdk/include/sdk/os/file.h"))
print("\n--- sdk/calc/calc.h ---")
print(fetch("https://raw.githubusercontent.com/ClasspadDev/hollyhock-3/main/sdk/include/sdk/calc/calc.h"))
print("\n--- sdk/os/lcd.h ---")
print(fetch("https://raw.githubusercontent.com/ClasspadDev/hollyhock-3/main/sdk/include/sdk/os/lcd.h"))
