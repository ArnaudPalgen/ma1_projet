esp_mesh = Proto("ESP-MESH", "ESP-MESH Protocol")

dest_addr = ProtoField.ether("espmesh.dest", "Destination address", base.HEX)
src_addr = ProtoField.ether("espmesh.src", "Source address", base.HEX)
data = ProtoField.bytes("espmesh.data", "Data", base.NONE)
flags = ProtoField.bytes("espmesh.flags", "Flags", base.NONE)


esp_mesh.fields = {data, dest_addr, src_addr, flags}

local espmesh_PID = 0xeeee 

local data_data = Field.new("data.data")
local llc_pid = Field.new("llc.pid")

function esp_mesh.dissector(tvbuf, pinfo, tree)
    local llc_pid_ex = llc_pid()

    if llc_pid_ex == nil or data_data() == nil or llc_pid_ex.value ~= espmesh_PID then
        return
    end
    pinfo.cols.protocol:set("esp-mesh")

    local data_tvb = data_data().range()
    local esp_mesh_tree = tree:add(esp_mesh, data_tvb(0, data_tvb:len()))

    esp_mesh_tree:add(dest_addr, data_tvb(8, 6))
    esp_mesh_tree:add(src_addr, data_tvb(14, 6))
    esp_mesh_tree:add(flags, data_tvb(20, 8))
    if data_tvb:len() > 28 then
        esp_mesh_tree:add(data, data_tvb(28))
    end
end

register_postdissector(esp_mesh)