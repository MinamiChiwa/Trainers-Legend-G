yamlcpp = {
	source = path.join(dependencies.basePath, "yaml-cpp"),
}

function yamlcpp.import()
	links { "yamlcpp" }
	yamlcpp.includes()
end

function yamlcpp.includes()
	includedirs {
		path.join(yamlcpp.source, "include")
	}
end

function yamlcpp.project()
	project "yamlcpp"
		language "C"

		yamlcpp.includes()

		files {
			path.join(yamlcpp.source, "src/**.cpp"),
			path.join(yamlcpp.source, "src/**.h"),
			path.join(yamlcpp.source, "src/**.natvis"),
		}

		warnings "Off"
		kind "StaticLib"
end

table.insert(dependencies, yamlcpp)
