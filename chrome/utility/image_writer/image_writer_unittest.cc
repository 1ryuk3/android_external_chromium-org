// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <string>

#include "base/files/file_util.h"
#include "base/files/scoped_temp_dir.h"
#include "base/run_loop.h"
#include "chrome/utility/image_writer/error_messages.h"
#include "chrome/utility/image_writer/image_writer.h"
#include "chrome/utility/image_writer/image_writer_handler.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace image_writer {

using testing::_;
using testing::AnyNumber;
using testing::AtLeast;
using testing::Lt;

namespace {

const int64 kTestFileSize = 1 << 15;  // 32 kB
const int kTestPattern = 0x55555555;

class ImageWriterUtilityTest : public testing::Test {
 protected:
  virtual void SetUp() OVERRIDE {
    ASSERT_TRUE(temp_dir_.CreateUniqueTempDir());
    ASSERT_TRUE(base::CreateTemporaryFileInDir(temp_dir_.path(), &image_path_));
    ASSERT_TRUE(
        base::CreateTemporaryFileInDir(temp_dir_.path(), &device_path_));
  }

  virtual void TearDown() OVERRIDE {}

  void FillFile(const base::FilePath& path, int pattern) {
    scoped_ptr<char[]> buffer(new char[kTestFileSize]);
    memset(buffer.get(), pattern, kTestFileSize);

    ASSERT_TRUE(base::WriteFile(path, buffer.get(), kTestFileSize));
  }

  void FillDefault(const base::FilePath& path) { FillFile(path, kTestPattern); }

  base::FilePath image_path_;
  base::FilePath device_path_;

 private:
  base::MessageLoop message_loop_;
  base::ScopedTempDir temp_dir_;
};

class MockHandler : public ImageWriterHandler {
 public:
  MOCK_METHOD1(SendProgress, void(int64));
  MOCK_METHOD1(SendFailed, void(const std::string& message));
  MOCK_METHOD0(SendSucceeded, void());
  MOCK_METHOD1(OnMessageReceived, bool(const IPC::Message& message));
};

// This Mock has the additional feature that it will start verification when
// the write completes.
class VerifyingHandler : public MockHandler {
 public:
  VerifyingHandler() : image_writer_(NULL), verified_(false) {}

  virtual void SendSucceeded() OVERRIDE {
    MockHandler::SendSucceeded();
    if (!verified_) {
      image_writer_->Verify();
      verified_ = true;
    }
  }
  ImageWriter* image_writer_;

 private:
  bool verified_;
};

}  // namespace

TEST_F(ImageWriterUtilityTest, Getters) {
  MockHandler mock_handler;
  ImageWriter image_writer(&mock_handler, image_path_, device_path_);

  EXPECT_EQ(image_path_, image_writer.GetImagePath());
  EXPECT_EQ(device_path_, image_writer.GetDevicePath());
}

TEST_F(ImageWriterUtilityTest, WriteSuccessful) {
  MockHandler mock_handler;
  ImageWriter image_writer(&mock_handler, image_path_, device_path_);

  EXPECT_CALL(mock_handler, SendProgress(_)).Times(AnyNumber());
  EXPECT_CALL(mock_handler, SendProgress(kTestFileSize)).Times(1);
  EXPECT_CALL(mock_handler, SendProgress(0)).Times(1);
  EXPECT_CALL(mock_handler, SendSucceeded()).Times(1);
  EXPECT_CALL(mock_handler, SendFailed(_)).Times(0);

  FillDefault(image_path_);
  image_writer.Write();
  base::RunLoop().RunUntilIdle();
}

TEST_F(ImageWriterUtilityTest, WriteInvalidImageFile) {
  MockHandler mock_handler;
  ImageWriter image_writer(&mock_handler, image_path_, device_path_);

  EXPECT_CALL(mock_handler, SendProgress(_)).Times(0);
  EXPECT_CALL(mock_handler, SendSucceeded()).Times(0);
  EXPECT_CALL(mock_handler, SendFailed(error::kOpenImage)).Times(1);

  ASSERT_TRUE(base::DeleteFile(image_path_, false));
  image_writer.Write();
  base::RunLoop().RunUntilIdle();
}

TEST_F(ImageWriterUtilityTest, WriteInvalidDeviceFile) {
  MockHandler mock_handler;
  ImageWriter image_writer(&mock_handler, image_path_, device_path_);

  EXPECT_CALL(mock_handler, SendProgress(_)).Times(0);
  EXPECT_CALL(mock_handler, SendSucceeded()).Times(0);
  EXPECT_CALL(mock_handler, SendFailed(error::kOpenDevice)).Times(1);

  ASSERT_TRUE(base::DeleteFile(device_path_, false));
  image_writer.Write();
  base::RunLoop().RunUntilIdle();
}

TEST_F(ImageWriterUtilityTest, VerifySuccessful) {
  MockHandler mock_handler;
  ImageWriter image_writer(&mock_handler, image_path_, device_path_);

  EXPECT_CALL(mock_handler, SendProgress(_)).Times(AnyNumber());
  EXPECT_CALL(mock_handler, SendProgress(kTestFileSize)).Times(1);
  EXPECT_CALL(mock_handler, SendProgress(0)).Times(1);
  EXPECT_CALL(mock_handler, SendSucceeded()).Times(1);
  EXPECT_CALL(mock_handler, SendFailed(_)).Times(0);

  FillDefault(image_path_);
  FillDefault(device_path_);

  image_writer.Verify();

  base::RunLoop().RunUntilIdle();
}

TEST_F(ImageWriterUtilityTest, VerifyInvalidImageFile) {
  MockHandler mock_handler;
  ImageWriter image_writer(&mock_handler, image_path_, device_path_);

  EXPECT_CALL(mock_handler, SendProgress(_)).Times(0);
  EXPECT_CALL(mock_handler, SendSucceeded()).Times(0);
  EXPECT_CALL(mock_handler, SendFailed(error::kOpenImage)).Times(1);

  ASSERT_TRUE(base::DeleteFile(image_path_, false));

  image_writer.Verify();

  base::RunLoop().RunUntilIdle();
}

TEST_F(ImageWriterUtilityTest, VerifyInvalidDeviceFile) {
  MockHandler mock_handler;
  ImageWriter image_writer(&mock_handler, image_path_, device_path_);

  EXPECT_CALL(mock_handler, SendProgress(_)).Times(0);
  EXPECT_CALL(mock_handler, SendSucceeded()).Times(0);
  EXPECT_CALL(mock_handler, SendFailed(error::kOpenDevice)).Times(1);

  ASSERT_TRUE(base::DeleteFile(device_path_, false));

  image_writer.Verify();

  base::RunLoop().RunUntilIdle();
}

TEST_F(ImageWriterUtilityTest, VerifyEmptyDevice) {
  MockHandler mock_handler;
  ImageWriter image_writer(&mock_handler, image_path_, device_path_);

  EXPECT_CALL(mock_handler, SendProgress(_)).Times(AnyNumber());
  EXPECT_CALL(mock_handler, SendSucceeded()).Times(0);
  EXPECT_CALL(mock_handler, SendFailed(error::kReadDevice)).Times(1);

  FillDefault(image_path_);
  image_writer.Verify();

  base::RunLoop().RunUntilIdle();
}

TEST_F(ImageWriterUtilityTest, VerifyFailed) {
  MockHandler mock_handler;
  ImageWriter image_writer(&mock_handler, image_path_, device_path_);

  EXPECT_CALL(mock_handler, SendProgress(_)).Times(AnyNumber());
  EXPECT_CALL(mock_handler, SendSucceeded()).Times(0);
  EXPECT_CALL(mock_handler, SendFailed(error::kVerificationFailed)).Times(1);

  FillDefault(image_path_);
  FillFile(device_path_, ~kTestPattern);
  image_writer.Verify();

  base::RunLoop().RunUntilIdle();
}

TEST_F(ImageWriterUtilityTest, WriteWithVerifySuccessful) {
  VerifyingHandler mock_handler;
  ImageWriter image_writer(&mock_handler, image_path_, device_path_);

  mock_handler.image_writer_ = &image_writer;

  EXPECT_CALL(mock_handler, SendProgress(_)).Times(AnyNumber());
  EXPECT_CALL(mock_handler, SendProgress(kTestFileSize)).Times(2);
  EXPECT_CALL(mock_handler, SendProgress(0)).Times(2);
  EXPECT_CALL(mock_handler, SendSucceeded()).Times(2);
  EXPECT_CALL(mock_handler, SendFailed(_)).Times(0);

  FillDefault(image_path_);

  image_writer.Write();

  base::RunLoop().RunUntilIdle();
}

}  // namespace image_writer
