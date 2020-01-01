# Dotnet-VCMP runtime host

> **Introduction to Dotnet-VCMP**
> 
> Dotnet-VCMP is a collection of interlinked software libraries that allow efficient and safe scripting of VCMP servers on the .NET Core platform.



## Abstract

This project is a VCMP plugin that, upon being initialized by a parent VCMP server, hosts a .NET Core runtime and forwards the functions, events and info structs received to a function called `EliteKillerz.DotnetVcmp.RuntimeClient.Bootstrap`, which is implemented in managed C# code and part of another project in the Dotnet-VCMP family.

## Configuration

The following configuration keys are supported as key-value pairs in a `server.cfg` file delimited by the first space character:

| **Key**     | **Value**                                                                                 |
| ----------- | ----------------------------------------------------------------------------------------- |
| `dotnetrtc` | Path to the `{assembly name}.runtimeconfig.json` file of the primary assembly to execute. |
| `dotnetasm` | Path to the `{assembly name}.dll` file of the primary assembly to execute.                |
