
// This successfully compiles SFML with C++20 header units. Except header units from prebuilt libraries vulkan, glad and
// minimp3, and, from directories as src/Win32/, all header units were successfully compiled. Adding  a header-unit from
// one of these causes compilation error. Not investigating for now.

#include "Configure.hpp"

template <typename... T>
void initializeTargets(TargetType targetType, DSC<CppSourceTarget> *target, const bool win32, T... targets)
{
    CppSourceTarget &t = target->getSourceTarget();
    string str = getLastNameAfterSlash(removeDashCppFromName(target->getSourceTarget().name));
    t.privateIncludes("src")
        .moduleDirectoriesRE("src/SFML/" + str, ".*cpp")
        .privateHUDirectories("src/SFML/" + str)
        .publicHUDirectories("include/SFML/" + str);

    if (win32)
    {
        t.moduleDirectoriesRE("src/SFML/" + str + "/Win32", ".*cpp").privateHUDirectories("src/SFML/" + str + "/Win32");
    }
    if (targetType == TargetType::LIBRARY_SHARED)
    {
        transform(str.begin(), str.end(), str.begin(), ::toupper);
        str += "_EXPORTS";
        t.privateCompileDefinition("SFML_" + str);
    }
    else
    {
        t.privateCompileDefinition("SFML_STATIC");
    }

    if constexpr (sizeof...(targets))
    {
        initializeTargets(targetType, targets...);
    }
}

void configurationSpecification(Configuration &configuration)
{
    configuration.assign(Define{"SFML_SYSTEM_WINDOWS"});
    configuration.archiving = true;

    DSC<CppSourceTarget> &stdhu = configuration.getCppObjectDSC("stdhu");

    stdhu.getSourceTarget().makeReqInclsUseable().publicIncludes("include");;
    stdhu.getLinkOrArchiveTarget().usageRequirementLibraryDirectories =
        stdhu.getLinkOrArchiveTarget().requirementLibraryDirectories;

    configuration.compilerFeatures.reqIncls.clear();
    configuration.prebuiltBasicFeatures.requirementLibraryDirectories.clear();

    const string winStaticLibsDir = srcNode->filePath + "/extlibs/libs-msvc-universal/x64/";

    DSC<CppSourceTarget> &openal32 = configuration.getCppStaticDSC_P("openal32", winStaticLibsDir);
    openal32.getSourceTarget().publicHUIncludes("extlibs/headers/AL");

    DSC<CppSourceTarget> &flac = configuration.getCppStaticDSC_P("flac", winStaticLibsDir);
    flac.getSourceTarget().publicCompileDefinition("FLAC__NO_DLL").publicHUIncludes("extlibs/headers/FLAC");

    DSC<CppSourceTarget> &freetype = configuration.getCppStaticDSC_P("freetype", winStaticLibsDir);

    DSC<CppSourceTarget> &glad = configuration.getCppObjectDSC("glad");
    glad.getSourceTarget().publicHUIncludes("extlibs/headers/glad/include");

    DSC<CppSourceTarget> &minimp3 = configuration.getCppObjectDSC("minimp3");
    minimp3.getSourceTarget().publicHUIncludes("extlibs/headers/minimp3");

    DSC<CppSourceTarget> &ogg = configuration.getCppStaticDSC_P("ogg", winStaticLibsDir);
    ogg.getSourceTarget()
        .publicHUDirectories("extlibs/headers/ogg")
        .privateIncludes("extlibs/headers")
        .interfaceIncludes("extlibs/headers");

    DSC<CppSourceTarget> &stb_image = configuration.getCppObjectDSC("stb_image");
    stb_image.getSourceTarget().publicHUIncludes("extlibs/headers/stb_image");

    DSC<CppSourceTarget> &vorbis = configuration.getCppStaticDSC_P("vorbis", winStaticLibsDir).privateLibraries(&ogg);
    vorbis.getSourceTarget()
        .interfaceCompileDefinition("OV_EXCLUDE_STATIC_CALLBACKS")
        .publicHUIncludes("extlibs/headers/vorbis");

    DSC<CppSourceTarget> &vorbisenc = configuration.getCppStaticDSC_P("vorbisenc", winStaticLibsDir);

    DSC<CppSourceTarget> &vorbisfile = configuration.getCppStaticDSC_P("vorbisfile", winStaticLibsDir);

    DSC<CppSourceTarget> &vulkan = configuration.getCppObjectDSC("vulkan");
    vulkan.getSourceTarget().publicHUIncludes("extlibs/headers/vulkan");

    DSC<CppSourceTarget> &system =
        configuration.getCppTargetDSC("system", configuration.targetType).privateLibraries(&stdhu);
    system.getSourceTarget().publicHUDirectories("src/SFML/system");

    DSC<CppSourceTarget> &network = configuration.getCppTargetDSC("network", configuration.targetType)
                                        .publicLibraries(&system)
                                        .privateLibraries(&stdhu, &glad);

    DSC<CppSourceTarget> &window = configuration.getCppTargetDSC("window", configuration.targetType)
                                       .publicLibraries(&system)
                                       .privateLibraries(&stdhu, &glad, &vulkan);

    // Sources are specified considering SFML_OPENGL_ES = true; Otherwise two files shouldn't be specified EGLCheck.pp
    // and EglContext.cpp. Better if these files are moved to a new directory, so moduleDirectoriesRE or
    // SOURCE_DIRECTORIES_RG could be conveniently used.
    initializeTargets(configuration.targetType, &window, true);

    // SFML_USE_DRM = false
    // OpenGL library is set by two variables OPENGL_INCLUDE_DIR and OPEN_gl_LIBRARY
    // Where are these variables set.
    // winmm and gdi32 are being linked

    DSC<CppSourceTarget> &graphics = configuration.getCppTargetDSC("graphics", configuration.targetType)
                                         .publicLibraries(&window)
                                         .privateLibraries(&stdhu, &freetype, &glad, &stb_image);

    CppSourceTarget &graphicsCpp = graphics.getSourceTarget();
    // graphicsCpp.reqIncls.clear();
    graphicsCpp.privateHUIncludes("extlibs/headers/freetype2").privateCompileDefinition("STBI_FAILURE_USERMSG");
    initializeTargets(configuration.targetType, &graphics, false);

    graphicsCpp.removeModuleFile("src/SFML/Graphics/Font.cpp").sourceFiles("src/SFML/Graphics/Font.cpp");
    // legacy_stdio_definitions.lib   --> Not declared or mentioned anywhere else
    // Freetype

    /*DSC<CppSourceTarget> &audio =
        configuration.getCppTargetDSC("audio", configuration.targetType)
            .publicLibraries(&system)
            .privateLibraries(&stdhu, &openal32, &vorbis, &vorbisenc, &vorbisfile, &flac, &minimp3);

    CppSourceTarget &audioCpp = audio.getSourceTarget();

    audioCpp.moduleDirectoriesRE("src/SFML/audio", ".*cpp").privateIncludes("extlibs/headers");*/

    initializeTargets(configuration.targetType, &system, true, &network, true);

    configuration.archiving = false;

    DSC<CppSourceTarget> &example = configuration.getCppExeDSC("Example").privateLibraries(&graphics, &stdhu);
    example.getSourceTarget().moduleFiles("examples/win32/Win32.cpp");
    if (configuration.name == "hu")
    {
        example.getSourceTarget().privateCompileDefinition("USE_HEADER_IMPORT");
    }
    if (configuration.targetType != TargetType::LIBRARY_SHARED)
    {
        window.getSourceTarget().privateCompileDefinition("SFML_STATIC");
        example.getSourceTarget().privateCompileDefinition("SFML_STATIC");
    }
    else
    {
        window.getSourceTarget().privateCompileDefinition("SFML_WINDOW_EXPORTS");
    }

    getRoundZeroUpdateBTarget(
        [&](Builder &builder, BTarget &bTarget) {
            if (atomic_ref(bTarget.fileStatus).load() && bTarget.realBTargets[0].exitStatus == EXIT_SUCCESS)
            {
                create_directory(path(example.getLinkOrArchiveTarget().outputFileNode->filePath).parent_path() /
                                 "resources");
                std::filesystem::copy(srcNode->filePath + "/examples/win32/resources",
                                      path(example.getLinkOrArchiveTarget().outputFileNode->filePath).parent_path() /
                                          "resources",
                                      std::filesystem::copy_options::update_existing);
            }
        },
        example.getLinkOrArchiveTarget());

    for (LinkOrArchiveTarget *loat : configuration.linkOrArchiveTargets)
    {
        if (loat->OR(TargetType::EXECUTABLE, TargetType::LIBRARY_SHARED))
        {
            loat->requirementLinkerFlags +=
                "advapi32.lib gdi32.lib opengl32.lib user32.lib winmm.lib winmm.Lib ws2_32.lib";
        }
    }
}

void buildSpecification()
{
    // This tries to build SFML similar to the current CMakeLists.txt. Currently, only Windows build is supported.
    getConfiguration("conventional").assign(CxxSTD::V_LATEST, TargetType::LIBRARY_SHARED, TreatModuleAsSource::YES);

    // Drop-in Replacement of header-files with header-units. Results in 2x incremental compilation speed-up of
    // Win32.cpp file. The caveat however is that I could not make shared libraries work with C++ 20 header-units. I
    // have a doubt that it is because of
    // https://developercommunity.visualstudio.com/t/Cannot-easily-link-C20-modules-as-DLLs/10202062?sort=votes&viewtype=all&page=3
    // Build succeeded with static libraries instead
    getConfiguration("drop-in").assign(CxxSTD::V_LATEST, TargetType::LIBRARY_STATIC, TreatModuleAsSource::NO,
                                       TranslateInclude::YES);

    // In this configuration, USE_HEADER_IMPORT definition will be set for Example target because of which Win32.cpp
    // imports only 3 header units. If TranslateInclude::YES would had been provided it would had imported 150 instead.

    // That is because if e.g. header-file main.cpp includes A.hpp which includes B.hpp. And TranslateInclude::YES
    // is used, then, B.hpp will be compiled first, then A.hpp, and main.cpp will depend on both A.hpp and B.hpp. But,
    // if TranslateInclude::YES is not used and main.cpp explicitly imports A.hpp which includes B.hpp, then, A.hpp will
    // be compiled with B.hpp contents and main.cpp will only depend on A.hpp.

    // This configuration was 3.5x faster which shows that importing aggregate header-unit is better than
    // importing multiple smaller header-units.
    getConfiguration("hu").assign(CxxSTD::V_LATEST, TargetType::LIBRARY_STATIC, TreatModuleAsSource::NO);

    // Run this benchmark.exe, this will execute hbuild in Example-cpp in the thrice configurations 10 times and prints
    // the average time. Please note that hbuild is executed in Example-cpp dir, so the source-file is recompiled but is
    // not relinked. Otherwise, results could had been different as 2 configurations link major libraries statically,
    // while one links them dynamically.
    getCppExeDSC("benchmark").getSourceTarget().sourceFiles("benchmark.cpp");

    selectiveConfigurationSpecification(&configurationSpecification);
}

MAIN_FUNCTION