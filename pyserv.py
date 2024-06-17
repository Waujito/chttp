import asyncio
import sys
from asyncio import StreamReader, StreamWriter

async def abort_conn(writer: StreamWriter):
    print("Connection abort!")
    writer.close()
    await writer.wait_closed()

async def server_conn(reader: StreamReader, writer: StreamWriter):
    # rawdata = bytearray()
    while True:
        if reader.at_eof():
            break

        # Read request
        reqline = (await reader.readline()).decode("utf-8")
        if not reqline.endswith("\r\n"):
            return await abort_conn(writer)

        reqline = reqline[:-2]

        reqs = reqline.split(" ")
        try:
            method = reqs[0]
            path = reqs[1]
            proto = reqs[2]
        except:
            return await abort_conn(writer)

        print(f"Got new request with method: {method}, path: {path}, and proto: {proto}")

        # Read headers
        headers = dict()
        while not reader.at_eof():
            nline = (await reader.readline()).decode("utf-8")

            if nline == "\r\n":
                break
            elif not nline.endswith('\r\n'):
                return await abort_conn(writer)
            else:
                nline = nline[:-2]

            nls = nline.split(":")

            if len(nls) < 2:
                return await abort_conn(writer)

            headerName = nls[0]
            headerValue = nls[1:]
            if len(headerValue) == 1:
                headerValue = headerValue[0]
            else:
                headerValue = ":".join(headerValue)

            headerName = headerName.strip()
            headerValue = headerValue.strip()

            headers[headerName] = headerValue

        # Read body
        handleBody = "Content-Length" in headers

        print(headers)

        if handleBody:
            bodyLength: str = headers["Content-Length"]
            if type(bodyLength) == str and bodyLength.isdigit():
                bodyLength = int(bodyLength)
            else:
                return await abort_conn(writer)

            bodyBuf = await reader.readexactly(bodyLength)

            print(bodyBuf)

    writer.close()
    await writer.wait_closed()

async def main():
    server = await asyncio.start_server(server_conn, 'localhost', 8888)

    async with server:
        await server.serve_forever()

asyncio.run(main())
    
