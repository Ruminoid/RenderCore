# Ruminoid Render Core

ASS Render Core for LIVE, SubLight, Trimmer and other Ruminoid products.

## Usage

You can use this library in C++ or in .NET.

### C++

Clone this repo as a submodule and add the submodule path to the IncludeDir of your project. Then see the "Build" section below to build the library. Finally, add the output directory to the LibraryDir of your project.

### .NET

We suggest you to use the [Ruminoid Renderer](https://github.com/Ruminoid/Renderer) repo as a submodule.

But if you want to interop directly with the dll library, you can use Ruminoid.Libraries NuGet package.

```ps1
Install-Package Ruminoid.Libraries
```

Please notice that this package requires `NuGet 3.3` or higher (in other words, `PackageReference`). If your project uses packages.config, please [migrate your packages to PackageReference](https://docs.microsoft.com/en-us/nuget/consume-packages/migrate-packages-config-to-package-reference).

## Build

Simply run the following scripts in the `Developer Command Prompt for Visual Studio` and you'll get a fresh dll file.

```sh
pip install conan
conan remote add charliejiang https://api.bintray.com/conan/charliejiang/conan
cmake .
```

## LICENSE

MIT
