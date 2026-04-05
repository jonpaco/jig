import { findMainActivity, patchSmali, resolveSmaliPath } from '../src/android/inject';
import fs from 'fs';
import path from 'path';
import os from 'os';

describe('findMainActivity', () => {
  it('extracts the launcher activity from a manifest', () => {
    const manifest = `<?xml version="1.0" encoding="utf-8"?>
<manifest xmlns:android="http://schemas.android.com/apk/res/android"
    package="com.example.myapp">
    <application android:label="MyApp">
        <activity android:name="com.example.myapp.SplashActivity" />
        <activity android:name="com.example.myapp.MainActivity">
            <intent-filter>
                <action android:name="android.intent.action.MAIN" />
                <category android:name="android.intent.category.LAUNCHER" />
            </intent-filter>
        </activity>
    </application>
</manifest>`;

    expect(findMainActivity(manifest)).toBe('com.example.myapp.MainActivity');
  });

  it('throws when no launcher activity is found', () => {
    const manifest = `<?xml version="1.0" encoding="utf-8"?>
<manifest xmlns:android="http://schemas.android.com/apk/res/android"
    package="com.example.myapp">
    <application>
        <activity android:name="com.example.myapp.MainActivity" />
    </application>
</manifest>`;

    expect(() => findMainActivity(manifest)).toThrow('Could not find launcher activity');
  });
});

describe('patchSmali', () => {
  let tmpDir: string;

  beforeEach(() => {
    tmpDir = fs.mkdtempSync(path.join(os.tmpdir(), 'jig-smali-'));
  });

  afterEach(() => {
    fs.rmSync(tmpDir, { recursive: true });
  });

  it('creates <clinit> when none exists', () => {
    const smaliPath = path.join(tmpDir, 'MainActivity.smali');
    fs.writeFileSync(smaliPath, `.class public Lcom/example/MainActivity;
.super Landroid/app/Activity;

.method public constructor <init>()V
    .locals 0
    invoke-direct {p0}, Landroid/app/Activity;-><init>()V
    return-void
.end method
`);

    patchSmali(smaliPath);
    const result = fs.readFileSync(smaliPath, 'utf-8');

    expect(result).toContain('.method static constructor <clinit>()V');
    expect(result).toContain('const-string v0, "jig"');
    expect(result).toContain('invoke-static {v0}, Ljava/lang/System;->loadLibrary(Ljava/lang/String;)V');
    expect(result).toContain('return-void');
  });

  it('injects into existing <clinit>', () => {
    const smaliPath = path.join(tmpDir, 'MainActivity.smali');
    fs.writeFileSync(smaliPath, `.class public Lcom/example/MainActivity;
.super Landroid/app/Activity;

.method static constructor <clinit>()V
    .locals 1
    sget-object v0, Lcom/example/SomeClass;->FIELD:Ljava/lang/String;
    return-void
.end method
`);

    patchSmali(smaliPath);
    const result = fs.readFileSync(smaliPath, 'utf-8');

    expect(result).toContain('const-string v0, "jig"');
    expect(result).toContain('invoke-static {v0}, Ljava/lang/System;->loadLibrary(Ljava/lang/String;)V');
    // Original code still present
    expect(result).toContain('sget-object v0');
  });

  it('is idempotent — does not double-patch', () => {
    const smaliPath = path.join(tmpDir, 'MainActivity.smali');
    fs.writeFileSync(smaliPath, `.class public Lcom/example/MainActivity;
.super Landroid/app/Activity;

.method public constructor <init>()V
    .locals 0
    return-void
.end method
`);

    patchSmali(smaliPath);
    const first = fs.readFileSync(smaliPath, 'utf-8');
    patchSmali(smaliPath);
    const second = fs.readFileSync(smaliPath, 'utf-8');

    expect(first).toBe(second);
  });
});

describe('resolveSmaliPath', () => {
  let tmpDir: string;

  beforeEach(() => {
    tmpDir = fs.mkdtempSync(path.join(os.tmpdir(), 'jig-resolve-'));
  });

  afterEach(() => {
    fs.rmSync(tmpDir, { recursive: true });
  });

  it('finds smali in primary smali/ directory', () => {
    const smaliFile = path.join(tmpDir, 'smali', 'com', 'example', 'MainActivity.smali');
    fs.mkdirSync(path.dirname(smaliFile), { recursive: true });
    fs.writeFileSync(smaliFile, '.class public Lcom/example/MainActivity;');

    const result = resolveSmaliPath(tmpDir, 'com.example.MainActivity');
    expect(result).toBe(smaliFile);
  });

  it('finds smali in smali_classes2/ directory', () => {
    const smaliFile = path.join(tmpDir, 'smali_classes2', 'com', 'example', 'MainActivity.smali');
    fs.mkdirSync(path.dirname(smaliFile), { recursive: true });
    fs.writeFileSync(smaliFile, '.class public Lcom/example/MainActivity;');

    // Also create empty smali/ so it's searched first
    fs.mkdirSync(path.join(tmpDir, 'smali'), { recursive: true });

    const result = resolveSmaliPath(tmpDir, 'com.example.MainActivity');
    expect(result).toBe(smaliFile);
  });

  it('throws when smali file not found', () => {
    fs.mkdirSync(path.join(tmpDir, 'smali'), { recursive: true });
    expect(() => resolveSmaliPath(tmpDir, 'com.example.Missing')).toThrow('Smali file not found');
  });
});
