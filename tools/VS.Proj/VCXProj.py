import xml.etree.ElementTree as ET
import uuid
import os
from pathlib import Path


class VCXProj:
	def __init__(self, projName, dylib, path):
		self.ProjName = projName
		self.ProjSource = '..'
		self.ConfigurationType = dylib #"StaticLibrary","DynamicLibrary"
		self.PlatformToolset = "v110"

		self.Headers = []
		self.Sources = []

		self.ProjPath = os.path.abspath(os.path.join(os.getcwd(), path))

		self.OutDir = '..\\lib';
		self.ImportLibrary = ''
		self.Subsystem = "Windows"
		self.CharacterSet = "MultiByte"
		
		#print(self.ProjPath)

		self.Platforms = {
			"Win32": {
				"Debug": {
					"Macros": ["WIN32", "_DEBUG"],
					"Optimization": "Disabled",
					"Librarys": []
				},
				"Release": {
					"Macros": ["WIN32", "NDEBUG"],
					"Optimization": "MaxSpeed",
					"Librarys": []
				}
			},
			"x64": {
				"Debug": {
					"Macros": ["WIN64", "_DEBUG"],
					"Optimization": "Disabled",
					"Librarys": []
				},
				"Release": {
					"Macros": ["WIN64", "NDEBUG"],
					"Optimization": "MaxSpeed",
					"Librarys": []
				}
			}
		}

		self.IncludePath = []
		self.LibraryPath = []

		try:
			os.makedirs(self.ProjPath)
		except FileExistsError:
			pass

	def get_parent_dirs(self,dirs,paths):
		path = Path(paths)
		current = path.parent
		while current != current.parent:
			dirs.add(str(current))
			current = current.parent

	def extract_dirs(self) -> list[str]:
		dirs = set()
		for path in self.Sources:
			self.get_parent_dirs(dirs,path)
		for path in self.Headers:
			self.get_parent_dirs(dirs,path)

		return sorted(dirs)

	def addIncludePath(self, path: list[str]):
		self.IncludePath.extend(path)

	def addLibraryPath(self, path: list[str]):
		self.LibraryPath.extend(path)

	def addSource(self, sources: list[str]):
		self.Sources.extend(sources)

	def addHeader(self, headers: list[str]):
		self.Headers.extend(headers)

	def addMacros(self, build, Macros: list[str]) :
		for platform, item in self.Platforms.items():
			if build in item and 'Macros' in item[build]:
				item[build]['Macros'].extend(Macros)

	def addLibrarys(self, build, Librarys: list[str]) :
		for platform, item in self.Platforms.items():
			if build in item and 'Librarys' in item[build]:
				item[build]['Librarys'].extend(Librarys)

	@classmethod
	def addPathFilter(cls, ItemGroup, Include):
		Filter = ET.SubElement(ItemGroup, "Filter", Include=Include)
		ET.SubElement(Filter,"UniqueIdentifier").text = str(uuid.uuid4())
		return Filter

	def addFileFilter(self, ItemGroup, files, Node):
		for file in files:
			node = ET.SubElement(ItemGroup, Node, Include=f"{self.ProjSource}\\{file.replace('/', '\\')}")
			ET.SubElement(node,"Filter").text = os.path.dirname(file).replace('/', '\\')

	def Filters(self):
		Project = ET.Element("Project", 
							ToolsVersion="4.0",
							xmlns="http://schemas.microsoft.com/developer/msbuild/2003")

		##############
		ItemGroup = ET.SubElement(Project,"ItemGroup")

		ET.SubElement(VCXProj.addPathFilter(ItemGroup,"assets"),"Extensions").text = "rc;ico;cur;bmp;dlg;rc2;rct;bin;rgs;gif;jpg;jpeg;jpe;resx;tiff;tif;png;wav;mfcribbon-ms"
		
		ET.SubElement(VCXProj.addPathFilter(ItemGroup,"src"),"Extensions").text = "cpp;c;cc;cxx;def;odl;idl;hpj;bat;asm;asmx"
		ET.SubElement(VCXProj.addPathFilter(ItemGroup,"include"),"Extensions").text = "h;hpp;hxx;hm;inl;inc;xsd"

		paths = self.extract_dirs()
		for file in paths:
			VCXProj.addPathFilter(ItemGroup,file.replace('/', '\\'))

		##############
		# 添加项目源文件
		ItemGroup = ET.SubElement(Project,"ItemGroup")
		self.addFileFilter(ItemGroup, self.Sources, "ClCompile")

		# 添加项目头文件
		ItemGroup = ET.SubElement(Project,"ItemGroup")
		self.addFileFilter(ItemGroup, self.Headers, "ClInclude")

		#
		filePath = os.path.abspath(os.path.join(self.ProjPath, f"{self.ProjName}.vcxproj.filters"))
		tree = ET.ElementTree(Project)
		tree.write(filePath, encoding="utf-8", xml_declaration=True)
		print(f"创建成功，查看:{filePath}")

	def build(self) :
		Project = ET.Element("Project", 
							DefaultTargets="Build",
							ToolsVersion="4.0",
							xmlns="http://schemas.microsoft.com/developer/msbuild/2003")
		# 添加项目配置
		ItemGroup = ET.SubElement(Project, "ItemGroup", Label="ProjectConfigurations")

		for platform,item in self.Platforms.items():
			for buildTtype,config in item.items():
				ProjectConfiguration = ET.SubElement(ItemGroup, "ProjectConfiguration",Include=f"{buildTtype}|{platform}")
				ET.SubElement(ProjectConfiguration, "Configuration").text = buildTtype
				ET.SubElement(ProjectConfiguration, "Platform").text = platform

		# 添加属性组
		props = ET.SubElement(Project, "PropertyGroup", Label="Globals")
		ET.SubElement(props, "ProjectGuid").text = str(uuid.uuid4())
		ET.SubElement(props, "RootNamespace").text = self.ProjName
		#ET.SubElement(props, "WindowsTargetPlatformVersion").text = "10.0.19041.0"
		ET.SubElement(props, "Keyword").text = "Win32Proj"

		# Import default props
		ET.SubElement(Project, "Import", Project="$(VCTargetsPath)\\Microsoft.Cpp.Default.props")

		for platform,item in self.Platforms.items():
			for buildTtype,config in item.items():
				ProjectConfiguration = ET.SubElement(Project, "PropertyGroup", Condition=f"'$(Configuration)|$(Platform)'=='{buildTtype}|{platform}'", Label="Configuration")
				ET.SubElement(ProjectConfiguration, "ConfigurationType").text = self.ConfigurationType
				ET.SubElement(ProjectConfiguration, "UseDebugLibraries").text = 'true' if config == 'Debug' else 'false'
				ET.SubElement(ProjectConfiguration, "CharacterSet").text = self.CharacterSet
				#ET.SubElement(ProjectConfiguration, "PlatformToolset").text = self.PlatformToolset

		# Import default props
		ET.SubElement(Project, "Import", Project="$(VCTargetsPath)\\Microsoft.Cpp.props")
		ET.SubElement(Project, "ImportGroup", Label="ExtensionSettings")

		for platform,item in self.Platforms.items():
			for buildTtype,config in item.items():
				v1 = ET.SubElement(Project, "ImportGroup", Label="PropertySheets", Condition=f"'$(Configuration)|$(Platform)'=='{buildTtype}|{platform}'")
				ET.SubElement(v1, "Import", 
									Project="$(UserRootDir)\\Microsoft.Cpp.$(Platform).user.props",
									Condition="exists('$(UserRootDir)\\Microsoft.Cpp.$(Platform).user.props')", 
									Label="LocalAppDataPlatform")

		ET.SubElement(Project, "PropertyGroup", Label="UserMacros")

		for platform,item in self.Platforms.items():
			for buildTtype,config in item.items():
				ProjectConfiguration = ET.SubElement(Project, "PropertyGroup", Condition=f"'$(Configuration)|$(Platform)'=='{buildTtype}|{platform}'")
				ET.SubElement(ProjectConfiguration, "OutDir").text = f"{self.OutDir}\\$(Platform)\\$(Configuration)\\"
				ET.SubElement(ProjectConfiguration, "IntDir").text = "$(Platform)\\$(Configuration)\\$(ProjectName)\\"
				ET.SubElement(ProjectConfiguration, "IncludePath").text = ";".join(self.IncludePath)+";$(IncludePath)"
				ET.SubElement(ProjectConfiguration, "LibraryPath").text = ";".join(self.LibraryPath)+";$(LibraryPath)"
				ET.SubElement(ProjectConfiguration, "LinkIncremental").text = 'true' if config == 'Release' else 'false'

		for platform,item in self.Platforms.items():
			for buildTtype,config in item.items():
				ProjectConfiguration = ET.SubElement(Project, "ItemDefinitionGroup", Condition=f"'$(Configuration)|$(Platform)'=='{buildTtype}|{platform}'")
				ClCompile = ET.SubElement(ProjectConfiguration, "ClCompile")
				ET.SubElement(ClCompile, "PrecompiledHeader")
				ET.SubElement(ClCompile, "WarningLevel").text = "Level3"

				Macros = config["Macros"]
				Librarys = config["Librarys"]
				Optimization = config["Optimization"]
				if Optimization == "MaxSpeed":
					ET.SubElement(ClCompile, "Optimization").text = Optimization
					ET.SubElement(ClCompile, "FunctionLevelLinking").text = "true"
					ET.SubElement(ClCompile, "IntrinsicFunctions").text = "true"
				else:
					ET.SubElement(ClCompile, "Optimization").text = Optimization

				ET.SubElement(ClCompile, "PreprocessorDefinitions").text = ";".join(Macros)+";%(PreprocessorDefinitions)"

				Link = ET.SubElement(ProjectConfiguration, "Link")
				ET.SubElement(Link, "Subsystem").text = self.Subsystem
				ET.SubElement(Link, "GenerateDebugInformation").text = "true"

				if self.ConfigurationType != "StaticLibrary":
					ET.SubElement(Link, "AdditionalDependencies").text = ";".join(Librarys)+";%(AdditionalDependencies)"
					ET.SubElement(Link, "ImportLibrary").text = f"{self.ImportLibrary}\\$(Platform)\\$(Configuration)\\$(ProjectName).lib"

				if buildTtype == "Release":
					ET.SubElement(Link, "EnableComdatfolding").text = "true"
					ET.SubElement(Link, "OptimizeReferences").text = "true"

		# 添加项目源文件
		ItemGroup = ET.SubElement(Project, "ItemGroup", Label="Sources")
		for file in self.Sources:
			file = file.replace('/', '\\')
			ET.SubElement(ItemGroup, "ClCompile", Include=f"{self.ProjSource}\\{file}")

		# 添加项目源文件
		ItemGroup = ET.SubElement(Project, "ItemGroup", Label="Headers")
		for file in self.Headers:
			file = file.replace('/', '\\')
			ET.SubElement(ItemGroup, "ClInclude", Include=f"{self.ProjSource}\\{file}")

		# Import targets
		ET.SubElement(Project, "Import", Project="$(VCTargetsPath)\\Microsoft.Cpp.targets")
		ET.SubElement(Project, "ImportGroup", Label="ExtensionTargets")

		filePath = os.path.abspath(os.path.join(self.ProjPath, f"{self.ProjName}.vcxproj"))
		tree = ET.ElementTree(Project)
		tree.write(filePath, encoding="utf-8", xml_declaration=True)
		print(f"创建成功，查看:{filePath}")

		self.Filters()