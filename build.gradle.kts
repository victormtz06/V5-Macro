import org.gradle.jvm.toolchain.JavaLanguageVersion
import org.jetbrains.kotlin.gradle.dsl.JvmTarget
import org.apache.tools.ant.filters.ReplaceTokens

plugins {
    alias(libs.plugins.kotlin)
    alias(libs.plugins.serialization)
    alias(libs.plugins.ksp)
    alias(libs.plugins.loom)
}

version = property("mod_version").toString()

repositories {
    mavenCentral()
    maven("https://maven.fabricmc.net")
    maven("https://jitpack.io")
    maven("https://maven.meteordev.org/releases")
    maven("https://maven.terraformersmc.com/releases")
    maven("https://repo.essential.gg/repository/maven-public")
    maven("https://repo.hypixel.net/repository/Hypixel/")
    maven("https://api.modrinth.com/maven")
}
val minecraftVersion = sc.current.version
val fabricApiVersion: String = sc.properties["deps.fabric_api"]
val universalcraftMinecraftVersion = if (minecraftVersion == "26.1.2") "26.1" else minecraftVersion

dependencies {
    // Minecraft-specific versions live in stonecutter.properties.toml.
    minecraft("com.mojang:minecraft:$minecraftVersion")
    implementation(libs.fabric.loader)
    implementation("net.fabricmc.fabric-api:fabric-api:$fabricApiVersion")
    implementation(libs.fabric.kotlin)

    implementation(libs.bundles.included) { include(this) }
    implementation("gg.essential:universalcraft-$universalcraftMinecraftVersion-fabric:${libs.versions.universalcraft.get()}") {
        include(this)
        exclude("gg.essential", "universalcraft-1.18.1-fabric")
    }
    implementation(libs.elementa) { include(this) }
    implementation(libs.vigilance) {
        include(this)
        exclude(group = "gg.essential", module = "elementa")
    }

    compileOnly(libs.sponge.mixin)
    ksp(project(":typing-generator"))
    // Discord IPC
    implementation("meteordevelopment:discord-ipc:1.1")
    include("meteordevelopment:discord-ipc:1.1")

    implementation(libs.skija.shared) { include(this) }
    implementation(libs.skija.types) { include(this) }

    // Mixin Extras
    implementation(libs.mixinextras) { include(this) }

    // Proxy support
    implementation("io.netty:netty-handler-proxy:4.2.7.Final")
    include("io.netty:netty-handler-proxy:4.2.7.Final")
    implementation("io.netty:netty-codec-socks:4.2.7.Final")
    include("io.netty:netty-codec-socks:4.2.7.Final")

    compileOnly(libs.hypixel.mod.api)
    implementation(libs.hypixel.modrinth.api) { include(this) }
}

loom {
    accessWidenerPath.set(rootProject.file("src/main/resources/ctjs.accesswidener"))
}

base {
    archivesName.set(property("archives_base_name") as String)
}

java {
    withSourcesJar()
    toolchain {
        languageVersion.set(JavaLanguageVersion.of(25))
    }
}

tasks {
    register("printVersion") {
        doLast { println(project.version) }
    }

    processResources {
        val mcVersion = minecraftVersion
        val flkVersion = libs.versions.fabric.kotlin.get()
        val fapiVersion = fabricApiVersion
        val loaderVersion = libs.versions.loader.get()
        val versionMixins = if (minecraftVersion == "26.1.2") {
            listOf("GuiHudMixin", "GuiScreenMixin", "LevelRendererMixin")
        } else {
            listOf(
                "CommandEncoderMixin", "GpuDeviceMixin", "GuiHudMixin", "GameRendererAccessor",
                "GuiScreenMixin", "LevelRendererMixin", "VulkanCommandEncoderMixin", "VulkanDeviceMixin",
            )
        }

        from(rootProject.file("typing-generator/src/main/resources")) {
            include("provided-types.properties")
        }

        inputs.property("version", project.version)
        inputs.property("minecraft_version", mcVersion)
        inputs.property("fabric_kotlin_version", flkVersion)
        inputs.property("fabric_api_version", fapiVersion)
        inputs.property("loader_version", loaderVersion)
        inputs.property("version_mixins", versionMixins.joinToString(","))

        filesMatching("fabric.mod.json") {
            expand(
                "version" to project.version,
                "minecraft_version" to mcVersion,
                "fabric_kotlin_version" to flkVersion,
                "fabric_api_version" to fapiVersion,
                "loader_version" to loaderVersion
            )
        }

        filesMatching("ctjs.mixins.json") {
            filter<ReplaceTokens>("tokens" to mapOf(
                "version_mixins" to versionMixins.joinToString("\",\n      \"")
            ))
        }

    }

    withType<JavaCompile>().configureEach {
        options.release.set(25)
    }

    kotlin {
        jvmToolchain(25)
        compilerOptions {
            jvmTarget.set(JvmTarget.JVM_25)
            freeCompilerArgs = listOf("-Xcontext-receivers")
        }
    }

    jar {
        archiveFileName.set(if (providers.gradleProperty("releaseBuild").isPresent) "V5-Loader-$minecraftVersion.jar" else "V5-Loader-DEV-$minecraftVersion.jar")
        exclude("typings.d.ts")
    }

    register<Copy>("generateTypings") {
        description = "Regenerates typing-generator/src/main/resources/typings.d.ts"
        group = "build"
        dependsOn("kspKotlin")
        from(layout.buildDirectory.dir("generated/ksp/main/resources")) {
            include("typings.d.ts")
        }
        into(rootProject.layout.projectDirectory.dir("typing-generator/src/main/resources"))
    }
}
