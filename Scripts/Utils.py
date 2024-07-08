﻿import sys
import os
import requests
import time
import urllib
import subprocess
import json
import tarfile

import platform
if platform.system() == "Windows":
    import winreg


from zipfile import ZipFile

# Windows
def FindVisualStudioPath(vswhere_path):
    try:
        result = subprocess.run([vswhere_path, '-products', '*', '-latest', '-format', 'json'], capture_output=True, text=True)
        if result.returncode == 0:
            installations = json.loads(result.stdout)
            if installations:
                return installations[0]['installationPath']
            else:
                print("No Visual Studio installations found.")
        else:
            print(f"Error: {result.stderr}")
    except Exception as e:
        print(f"[VSwhere] An error occurred: {e}")

def SetWindowsSystemEnvironmentVariable(variable_name, directory_path):
    key = winreg.OpenKey(winreg.HKEY_LOCAL_MACHINE, r"SYSTEM\CurrentControlSet\Control\Session Manager\Environment", 0, winreg.KEY_SET_VALUE)
    winreg.SetValueEx(key, variable_name, 0, winreg.REG_EXPAND_SZ, directory_path)
    winreg.CloseKey(key)

def GetWindowsSystemEnvironmentVariable(name):
    key = winreg.CreateKey(winreg.HKEY_LOCAL_MACHINE, r"System\CurrentControlSet\Control\Session Manage\Environment")
    try:
        return winreg.QueryValueEx(key, name)[0]
    except:
        return None

def GetWindowsUserEnvironmentVariable(name):
    key = winreg.CreateKey(winreg.HKEY_CURRENT_USER, r"Environment")
    try:
        return winreg.QueryValueEx(key, name)[0]
    except:
        return None
    
def AddNewWindowsSystemPathEnvironment(new_path):
    current_path = current_path = os.environ.get('PATH', '')
    if new_path not in current_path:
        new_path = f'{current_path};{new_path}'
        SetWindowsSystemEnvironmentVariable("Path", new_path)
        
    return new_path not in current_path
# !Windows

# Linux
def SetLinuxSystemEnvironmentVariable(variable_name, directory_path):
    os.environ[variable_name] = directory_path

    with open('/etc/environment', 'a') as file:
        file.write(f'\n{variable_name}="{directory_path}"\n')


def GetLinuxSystemEnviromentVariable(name):
    value = os.environ.get(name)
    if value is not None:
        return True
    
    try:
        with open('/etc/environment', 'r') as file:
            for line in file:
                if line.startswith(f'{name}='):
                    return line.split('=', 1)[1].strip().strip('"')
    except FileNotFoundError:
        pass

    return None

def GetLinuxUserEnvironmentVariable(name):
    return os.environ.get(name)

def AddNeLinuxSystemPathEnvironment(new_path):
    current_path = os.environ.get('PATH', '')
    if new_path not in current_path.split(':'):
        new_path = f'{current_path}:{new_path}'

        os.environ['PATH'] = new_path

        with open('/etc/environment', 'a') as file:
            file.write(f'\nPATH="{new_path}"\n')

        return True
    return False
# !Linux


def DownloadFile(url, filepath):
    path = filepath
    filepath = os.path.abspath(filepath)
    os.makedirs(os.path.dirname(filepath), exist_ok=True)
    if (type(url) is list):
        for url_option in url:
            print("Downloading", url_option)
            try:
                DownloadFile(url_option, filepath)
                return
            except urllib.error.URLError as e:
                print(f"URL Error encountered: {e.reason}. Proceeding with backup...\n\n")
                os.remove(filepath)
                pass
            except urllib.error.HTTPError as e:
                print(f"HTTP Error  encountered: {e.code}. Proceeding with backup...\n\n")
                os.remove(filepath)
                pass
            except:
                print(f"Something went wrong. Proceeding with backup...\n\n")
                os.remove(filepath)
                pass
        raise ValueError(f"Failed to download {filepath}")
    if not(type(url) is str):
        raise TypeError("Argument 'url' must be of type list or string")

    with open(filepath, 'wb') as f:
        headers = {'User-Agent': "Mozilla/5.0 (Macintosh Intel Mac Os X 10_15_4) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/83.0.4103.97 Safari/537.36"}
        response = requests.get(url, headers = headers, stream = True)
        total = response.headers.get('content-length')

        if total is None:
            f.write(response.content)
        else:
            downloaded = 0
            total = int(total)
            startTime = time.time()
            for data in response.iter_content(chunk_size = max(int(total/1000), 1024*1024)):
                downloaded += len(data)
                f.write(data)
                done = int(50*downloaded / total)
                percentage = (downloaded / total) * 100
                elapsedTime = time.time() - startTime
                avgKBPerSecond = (downloaded / 1024) / elapsedTime
                avgSpeedString = '{:.2f} KB/s'.format(avgKBPerSecond)
            if(avgKBPerSecond > 1024):
                avgMGPerSecond = avgKBPerSecond / 1024
                avgSpeedString = '{:.2f} MB/s'.format(avgMGPerSecond)
            sys.stdout.write('\r[{}{}] {:.2f}% ({})     '.format('█' * done, '.' * (50-done), percentage, avgSpeedString))
            sys.stdout.flush()
    sys.stdout.write('\n')

def ExtractArchiveFile(filepath, deleteArchiveFile=True):
    archFilepath = os.path.abspath(filepath)
    archFileLocation = os.path.dirname(archFilepath)

    if (platform.system() == "Windows"):
        zipFileContent = dict()
        zipFileContentSize = 0
        with ZipFile(archFilepath, 'r') as zipFileFolder:
            for name in zipFileFolder.namelist():
                zipFileContent[name] = zipFileFolder.getinfo(name).file_size
            zipFileContentSize = sum(zipFileContent.values())
            extractedContentSize = 0
            startTime = time.time()
            for zippedFileName, zippedFileSize in zipFileContent.items():
                UnzippedFilePath = os.path.abspath(f"{archFileLocation}/{zippedFileName}")
                os.makedirs(os.path.dirname(UnzippedFilePath), exist_ok = True)
                if os.path.isfile(UnzippedFilePath):
                    zipFileContentSize -= zippedFileSize
                else:
                    zipFileFolder.extract(zippedFileName, path = archFileLocation, pwd=None)
                    extractedContentSize += zippedFileSize
                try:
                    done = int(50*extractedContentSize/zipFileContentSize)
                    percentage = (extractedContentSize / zipFileContentSize) * 100
                except ZeroDivisionError:
                    done = 50
                    percentage = 100
                elapsedTime = time.time() - startTime
                try:
                    avgKBPerSecond = (extractedContentSize / 1024) / elapsedTime
                except ZeroDivisionError:
                    avgKBPerSecond = 0.0
                avgSpeedString = '{:.2f} KB/s'.format(avgKBPerSecond)
                if (avgKBPerSecond > 1024):
                    avgMBPerSecond = avgKBPerSecond / 1024
                    avgSpeedString = '{:.2f} MB/s'.format(avgMBPerSecond)
                sys.stdout.write('\r[{}{}] {:.2f}% ({})     '.format('█' * done, '.' * (50-done), percentage, avgSpeedString))
                sys.stdout.flush()
        sys.stdout.write('\n')
    elif (platform.system() == "Linux"):
        file = tarfile.open(archFilepath)
        file.extractall(archFileLocation)
        file.close()

    if deleteArchiveFile:
        os.remove(archFilepath) # delete zip file