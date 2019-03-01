
function lax_analysis(source)
    local laxs = {}
    local source_length = string.len(source)
    local off = 0

    while off <= source_length do
        local start_index, end_index 
        local lax = nil

        -- op ...
        if (lax == nil) then
            start_index, end_index, lax = string.find(source, '^[%s%c]*(%.%.%.)', off)
        end

        -- string
        if lax == nil then
            start_index, end_index, lax = string.find(source, '^[%s%c]*("[^"]*")', off)
        end

        -- field
        if lax == nil then
            start_index, end_index, lax = string.find(source, '^[%s%c]*(_*%a%w+)', off)
        end

        -- dir
        if lax == nil then
            start_index, end_index, lax = string.find(source, '^[%s%c]*(@_*%a%w+)', off)
        end

        -- var
        if lax == nil then
            start_index, end_index, lax = string.find(source, '^[%s%c]*($_*%a%w+)', off)
        end

        -- op
        if lax == nil then
            start_index, end_index, lax = string.find(source, '^[%s%c]*(%p)', off)
        end

        -- number
        if lax == nil then
            start_index, end_index, lax = string.find(source, '^[%s%c]*(%d+)', off)
        end

        if lax ~= nil then
            table.insert(laxs, lax)
            off = end_index + 1
        else
            break
        end
    end

    return laxs
end

