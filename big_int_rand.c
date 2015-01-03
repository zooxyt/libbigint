#include <time.h>
#include <stdlib.h>

static unsigned int rand_table[] = {
    47  , 73  , 116 , 45  , 69  , 61  , 202 , 144 , 192 , 235 ,
    201 , 170 , 139 , 77  , 228 , 117 , 32  , 215 , 9   , 178 ,
    214 , 193 , 64  , 200 , 58  , 132 , 89  , 60  , 63  , 141 ,
    35  , 234 , 76  , 95  , 20  , 182 , 173 , 190 , 68  , 229 ,
    27  , 28  , 226 , 143 , 186 , 86  , 138 , 54  , 75  , 242 ,
    90  , 42  , 211 , 15  , 100 , 1   , 254 , 243 , 134 , 156 ,
    218 , 26  , 24  , 187 , 128 , 14  , 175 , 53  , 67  , 246 ,
    230 , 167 , 236 , 146 , 18  , 23  , 177 , 213 , 142 , 74  ,
    118 , 147 , 203 , 159 , 112 , 196 , 171 , 249 , 240 , 56  ,
    16  , 244 , 169 , 70  , 3   , 191 , 150 , 57  , 126 , 30  ,
    10  , 160 , 206 , 37  , 109 , 25  , 6   , 66  , 46  , 210 ,
    157 , 212 , 145 , 2   , 39  , 204 , 72  , 224 , 250 , 88  ,
    104 , 155 , 52  , 108 , 105 , 81  , 85  , 151 , 93  , 103 ,
    184 , 83  , 34  , 255 , 51  , 239 , 4   , 162 , 222 , 59  ,
    22  , 161 , 12  , 91  , 50  , 199 , 101 , 216 , 80  , 119 ,
    164 , 71  , 82  , 107 , 251 , 13  , 129 , 94  , 44  , 96  ,
    225 , 8   , 223 , 135 , 153 , 165 , 174 , 220 , 102 , 238 ,
    154 , 197 , 33  , 149 , 41  , 19  , 140 , 40  , 247 , 114 ,
    195 , 78  , 43  , 168 , 233 , 209 , 148 , 180 , 237 , 253 ,
    185 , 166 , 11  , 98  , 198 , 241 , 133 , 21  , 207 , 248 ,
    219 , 245 , 36  , 172 , 55  , 152 , 188 , 84  , 125 , 189 ,
    163 , 127 , 120 , 205 , 124 , 5   , 31  , 122 , 110 , 217 ,
    181 , 87  , 137 , 115 , 131 , 252 , 7   , 183 , 111 , 176 ,
    231 , 65  , 194 , 0   , 113 , 79  , 49  , 97  , 99  , 179 ,
    121 , 158 , 29  , 17  , 106 , 227 , 38  , 62  , 123 , 130 ,
    92  , 221 , 136 , 208 , 48  , 232
};

#if defined (UNIX)
#include <unistd.h>
#include <fcntl.h>
static int random_fd = 0;
#endif

int rand_initialize()
{
#if defined(UNIX)
    random_fd = open("/dev/urandom", O_RDONLY);
    if (random_fd == -1) return -1;
#else
    srand((unsigned int)time(NULL));
#endif
    return 0;
}

int rand_uninitialize()
{
#if defined(UNIX)
    return close(random_fd);
#else
    srand((unsigned int)time(NULL));
#endif
    return 0;
}

#if defined(UNIX)
unsigned int rand_get_number(int byte)
{
    int idx;
    unsigned int value = 0;
    unsigned char part;
    for (idx = 0; idx < byte; idx++)
    {
        if (read(random_fd, &part, 1) != 1) return 0;
        value |= rand_table[part] << (idx << 3);
    }
    return value;
}
#else
unsigned int rand_get_number(int byte)
{
    int idx;
    unsigned int value = 0;
    unsigned int part;
    for (idx = 0; idx < byte; idx++)
    {
        part = rand_table[(unsigned int)(rand()) & 0xFF];
        value |= part << (idx << 3);
    }
    return value;
}
#endif

