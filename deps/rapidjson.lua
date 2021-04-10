rapidjson = {
	source = path.join(dependencies.basePath, "rapidjson"),
}

function rapidjson.import()
	rapidjson.includes()
end

function rapidjson.includes()
	includedirs {
		path.join(rapidjson.source, "include")
	}
end

function rapidjson.project()
end

table.insert(dependencies, rapidjson)
