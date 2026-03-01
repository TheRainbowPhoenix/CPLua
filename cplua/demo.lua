-- CPLua Draw Demo
calc.clear()

-- Draw a border
for x = 0, 319 do
    calc.set_pixel(x, 0, 0x0000) -- Black
    calc.set_pixel(x, 527, 0x0000)
end

for y = 0, 527 do
    calc.set_pixel(0, y, 0x0000)
    calc.set_pixel(319, y, 0x0000)
end

-- Draw a cool pattern in the center
for x = 50, 270 do
    for y = 100, 400 do
        local r = (x % 32) * 8
        local g = (y % 64) * 4
        local b = ((x+y) % 32) * 8

        -- Convert RGB888 to RGB565
        local color = ((r / 8) * 2048) + ((g / 4) * 32) + (b / 8)

        -- A little hole in the middle
        if (x > 140 and x < 180 and y > 230 and y < 270) then
            color = 0xFFFF -- White
        end

        calc.set_pixel(x, y, color)
    end
end

calc.refresh()
print("Demo drawn to screen successfully!")
