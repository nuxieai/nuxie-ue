pluginManagement {
  repositories {
    google()
    mavenCentral()
    gradlePluginPortal()
  }
}

dependencyResolutionManagement {
  repositoriesMode.set(RepositoriesMode.FAIL_ON_PROJECT_REPOS)
  repositories {
    providers.environmentVariable("NUXIE_ANDROID_MAVEN_REPO")
      .orNull
      ?.let { maven(url = uri(it)) }
    maven(url = uri(rootDir.resolve("../../.native/maven")))
    google()
    mavenCentral()
  }
}

rootProject.name = "nuxie-unreal-android"
include(":bridge")
