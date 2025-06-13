plugins {
    alias(libs.plugins.android.application)
}

android {
    namespace = "com.msun.ncap"
    compileSdk = 35

    defaultConfig {
        ndk {
            abiFilters.add ("armeabi-v7a")
            abiFilters.add ("arm64-v8a")
            abiFilters.add ("x86_64")
        }
        applicationId = "com.msun.ncap"
        minSdk = 26
        targetSdk = 35
        versionCode = 1
        versionName = "1.0"
        externalNativeBuild {
            cmake {
                arguments.add("-DNDK=/opt/homebrew/share/android-ndk")
            }
        }
    }

    buildTypes {
        release {
            isMinifyEnabled = false
            proguardFiles(
                getDefaultProguardFile("proguard-android-optimize.txt"),
                "proguard-rules.pro"
            )
        }
    }
    compileOptions {
        sourceCompatibility = JavaVersion.VERSION_11
        targetCompatibility = JavaVersion.VERSION_11
    }
    externalNativeBuild {
        cmake {
            path = file("src/main/c/CMakeLists.txt")
            version = "4.0.2"
        }
    }
    buildFeatures {
        viewBinding = true
    }
}

dependencies {
    implementation(libs.appcompat)
    implementation(libs.material)
    implementation(libs.constraintlayout)
    implementation("androidx.startup:startup-runtime:1.0.0")
}
