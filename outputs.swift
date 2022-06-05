import CoreMedia
import Foundation

class RTMPDelegate: Output {
  var url: URL?
  var rtmp = RTMP()
  var i = 0

  convenience init(url: URL?) {
    self.init()
    self.url = url
  }

  init() {
    RTMP_LogSetLevel(RTMP_LOGDEBUG)

    // Show librtmp version
    let version = RTMP_LibVersion()
    let versionString = String(format: "0x%08x", version) // RTMP_LibVersion: 0x00020300
    print("RTMP_LibVersion: \(versionString)")

    RTMP_Init(&rtmp)
  }

  func open() {
    RTMPx_SetupURL(&rtmp, url?.absoluteString)
    RTMP_EnableWrite(&rtmp)
    RTMP_Connect(&rtmp, nil)
    RTMP_ConnectStream(&rtmp, 0)
  }

  func close() {
    RTMPx_Close(&rtmp)
  }

  func onVideoMetadata(sps: Data, pps: Data) {
    var data = Array(repeating: 0x00, count: 256)

    data.withUnsafeMutableBufferPointer { buffer in
      var count = 0
      sps.withUnsafeBytes { spsBuffer in
        pps.withUnsafeBytes { ppsBuffer in
          print("sps: ", sps, "pps: ", pps)
          count = RTMPx_MakeVideoMetadataTag(spsBuffer.baseAddress, spsBuffer.count, ppsBuffer.baseAddress, ppsBuffer.count, buffer.baseAddress, buffer.count)
        }
      }
      RTMP_Write(&rtmp, buffer.baseAddress, Int32(count))
    }
  }

  func onVideoSample(_ sampleBuffer: CMSampleBuffer) {
    print("\(#file):\(#line) \(#function)")

    guard let dataBuffer = CMSampleBufferGetDataBuffer(sampleBuffer) else { return }
    let dataLength = CMBlockBufferGetDataLength(dataBuffer)
    print("on video sample: ", dataLength)
    var dataPointer: UnsafeMutablePointer<Int8>?
    CMBlockBufferGetDataPointer(dataBuffer, atOffset: 0, lengthAtOffsetOut: nil, totalLengthOut: nil, dataPointerOut: &dataPointer)

    let timestamp = i * 40
    i += 1
    var data = Data(count: 16 + dataLength)
    data.withUnsafeMutableBytes { buffer in
      let count = RTMPx_MakeVideoNALUTag(Int32(timestamp), dataPointer!, dataLength, buffer.baseAddress, buffer.count)

      RTMP_Write(&rtmp, buffer.baseAddress, Int32(count))
    }
    // print(data.hexString)
  }
}

class H264FileDelegate: Output {
  var url: URL?
  private var h264file: FileHandle?
  private let startCode: [Int8] = [0x00, 0x00, 0x00, 0x01]
  private let startCodeData: Data?

  init(url: URL) {
    self.url = url
    startCodeData = Data(bytes: startCode, count: 4)

    // let home = FileManager.default.homeDirectoryForCurrentUser
    // let clip = home.appendingPathComponent("/Desktop/output.h264")
    try? FileManager.default.removeItem(at: self.url!)
    if FileManager.default.createFile(atPath: self.url!.path, contents: nil, attributes: nil) {
      h264file = try? FileHandle(forWritingTo: url)
    }
  }

  func open() {}

  func close() {
    do { try h264file?.close() } catch {}
  }

  func onVideoMetadata(sps: Data, pps: Data) {
    guard let f = h264file else { return }

    f.write(startCodeData!)
    f.write(sps)
    f.write(startCodeData!)
    f.write(pps)

    print("write sps and pps")
  }

  func onVideoSample(_ sampleBuffer: CMSampleBuffer) {
    guard let f = h264file else { return }

    guard let dataBuffer = CMSampleBufferGetDataBuffer(sampleBuffer) else { return }
    var lengthAtOffset = 0
    var totalLength = 0
    var dataPointer: UnsafeMutablePointer<Int8>?
    if CMBlockBufferGetDataPointer(dataBuffer, atOffset: 0, lengthAtOffsetOut: &lengthAtOffset, totalLengthOut: &totalLength, dataPointerOut: &dataPointer) == noErr {
      var bufferOffset = 0
      let AVCCHeaderLength = 4

      while bufferOffset < (totalLength - AVCCHeaderLength) {
        var NALUnitLength: UInt32 = 0
        // first four character is NALUnit length
        memcpy(&NALUnitLength, dataPointer?.advanced(by: bufferOffset), AVCCHeaderLength)

        // big endian to host endian. in iOS it's little endian
        NALUnitLength = CFSwapInt32BigToHost(NALUnitLength)

        let data = NSData(bytes: dataPointer?.advanced(by: bufferOffset + AVCCHeaderLength), length: Int(NALUnitLength))
        // [UInt8]
        // let data = Data(bytes: dataPointer?.advanced(by: bufferOffset + AVCCHeaderLength), count: Int(NALUnitLength))

        f.write(startCodeData!)
        f.write(data as Data)
        print("write NALU")

        // move forward to the next NAL Unit
        bufferOffset += Int(AVCCHeaderLength)
        bufferOffset += Int(NALUnitLength)
      }
    }
  }
}
