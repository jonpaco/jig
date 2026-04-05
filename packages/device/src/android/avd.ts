import fs from 'fs';
import path from 'path';

export const AVD_PREFIX = 'jig-';

export interface AVDConfig {
  profileName: string;
  systemImage: string;
  ram: number;
  screen: string;
}

export function buildConfigIni(config: AVDConfig): string {
  const { profileName, systemImage, ram, screen } = config;

  const parts = systemImage.split(';');
  const apiLevel = parts[1]?.replace('android-', '') || '34';
  const target = parts[2] || 'google_apis';
  const abi = parts[3] || 'arm64-v8a';
  const imagePath = `system-images/${parts[1]}/${target}/${abi}/`;
  const isPlayStore = target === 'google_apis_playstore';

  const [width, height] = screen.split('x').map(Number);
  const avdName = `${AVD_PREFIX}${profileName}`;

  const lines = [
    `AvdId=${avdName}`,
    `PlayStore.enabled=${isPlayStore}`,
    `abi.type=${abi}`,
    `avd.ini.displayname=Jig ${profileName}`,
    `avd.ini.encoding=UTF-8`,
    `disk.dataPartition.size=6G`,
    `fastboot.chosenSnapshotFile=`,
    `fastboot.forceChosenSnapshotBoot=no`,
    `fastboot.forceColdBoot=no`,
    `fastboot.forceFastBoot=yes`,
    `hw.accelerometer=yes`,
    `hw.arc=false`,
    `hw.audioInput=yes`,
    `hw.battery=yes`,
    `hw.camera.back=none`,
    `hw.camera.front=none`,
    `hw.cpu.arch=${abi === 'x86_64' ? 'x86_64' : 'arm64'}`,
    `hw.cpu.ncore=4`,
    `hw.dPad=no`,
    `hw.device.manufacturer=Google`,
    `hw.device.name=pixel_6`,
    `hw.gps=yes`,
    `hw.gpu.enabled=yes`,
    `hw.gpu.mode=auto`,
    `hw.keyboard=yes`,
    `hw.lcd.density=420`,
    `hw.lcd.height=${height}`,
    `hw.lcd.width=${width}`,
    `hw.mainKeys=no`,
    `hw.ramSize=${ram}`,
    `hw.sdCard=no`,
    `hw.sensors.orientation=yes`,
    `hw.sensors.proximity=yes`,
    `hw.trackBall=no`,
    `image.sysdir.1=${imagePath}`,
    `runtime.network.latency=none`,
    `runtime.network.speed=full`,
    `showDeviceFrame=no`,
    `skin.dynamic=yes`,
    `skin.name=${width}x${height}`,
    `skin.path=${width}x${height}`,
    `tag.display=${isPlayStore ? 'Google Play' : 'Google APIs'}`,
    `tag.id=${target}`,
    `target=android-${apiLevel}`,
    `vm.heapSize=256`,
  ];

  return lines.join('\n') + '\n';
}

export function buildPointerIni(avdDirPath: string, apiLevel: string): string {
  const avdName = path.basename(avdDirPath, '.avd');
  return [
    `avd.ini.encoding=UTF-8`,
    `path=${avdDirPath}`,
    `path.rel=avd/${avdName}.avd`,
    `target=android-${apiLevel}`,
  ].join('\n') + '\n';
}

export function getAvdHome(): string {
  return (
    process.env.ANDROID_AVD_HOME ||
    path.join(process.env.HOME || process.env.USERPROFILE || '', '.android', 'avd')
  );
}

export function createAvd(config: AVDConfig): { avdDir: string; avdName: string } {
  const avdHome = getAvdHome();
  const avdName = `${AVD_PREFIX}${config.profileName}`;
  const avdDir = path.join(avdHome, `${avdName}.avd`);

  fs.mkdirSync(avdDir, { recursive: true });
  fs.writeFileSync(path.join(avdDir, 'config.ini'), buildConfigIni(config));

  const apiLevel = config.systemImage.split(';')[1]?.replace('android-', '') || '34';
  fs.writeFileSync(
    path.join(avdHome, `${avdName}.ini`),
    buildPointerIni(avdDir, apiLevel),
  );

  return { avdDir, avdName };
}

export function deleteAvd(profileName: string): void {
  const avdHome = getAvdHome();
  const avdName = `${AVD_PREFIX}${profileName}`;
  const avdDir = path.join(avdHome, `${avdName}.avd`);
  const iniFile = path.join(avdHome, `${avdName}.ini`);

  if (fs.existsSync(avdDir)) fs.rmSync(avdDir, { recursive: true });
  if (fs.existsSync(iniFile)) fs.unlinkSync(iniFile);
}

export function getExistingAvdAbi(profileName: string): string | null {
  const avdHome = getAvdHome();
  const avdName = `${AVD_PREFIX}${profileName}`;
  const configPath = path.join(avdHome, `${avdName}.avd`, 'config.ini');

  if (!fs.existsSync(configPath)) return null;

  const config = fs.readFileSync(configPath, 'utf-8');
  const abiMatch = config.match(/abi\.type=(.+)/);
  return abiMatch ? abiMatch[1].trim() : null;
}
