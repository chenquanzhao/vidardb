//  Copyright (c) 2019-present, VidarDB, Inc.  All rights reserved.
//  This source code is licensed under the BSD-style license found in the
//  LICENSE file in the root directory of this source tree. An additional grant
//  of patent rights can be found in the PATENTS file in the same directory.
//
//  Copyright (c) 2011-present, Facebook, Inc.  All rights reserved.
//  This source code is licensed under the BSD-style license found in the
//  LICENSE file in the root directory of this source tree. An additional grant
//  of patent rights can be found in the PATENTS file in the same directory.
//
// Copyright (c) 2011 The LevelDB Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file. See the AUTHORS file for names of contributors.

#pragma once
#include <stdint.h>
#include <limits>
#include <string>
#include <utility>
#include <vector>

#include "vidardb/flush_block_policy.h"
#include "vidardb/options.h"
#include "vidardb/status.h"
#include "table/table_builder.h"

namespace vidardb {

class BlockBuilder;
class BlockHandle;
class WritableFile;
struct BlockBasedTableOptions;

extern const uint64_t kBlockBasedTableMagicNumber;

class BlockBasedTableBuilder : public TableBuilder {
 public:
  // Create a builder that will store the contents of the table it is
  // building in *file. Does not close the file. It is up to the
  // caller to close the file after calling Finish().
  // @param compression_dict Data for presetting the compression library's
  //    dictionary, or nullptr.
  BlockBasedTableBuilder(
      const ImmutableCFOptions& ioptions,
      const BlockBasedTableOptions& table_options,
      const InternalKeyComparator& internal_comparator,
      const std::vector<std::unique_ptr<IntTblPropCollectorFactory>>*
          int_tbl_prop_collector_factories,
      uint32_t column_family_id, WritableFileWriter* file,
      const CompressionType compression_type,
      const CompressionOptions& compression_opts,
      const std::string* compression_dict,
      const std::string& column_family_name);

  // REQUIRES: Either Finish() or Abandon() has been called.
  ~BlockBasedTableBuilder();

  // Add key, value to the table being constructed.
  // REQUIRES: key is after any previously added key according to comparator.
  // REQUIRES: Finish(), Abandon() have not been called
  void Add(const Slice& key, const Slice& value) override;

  // Return non-ok iff some error has been detected.
  Status status() const override;

  // Finish building the table. Stops using the file passed to the
  // constructor after this function returns.
  // REQUIRES: Finish(), Abandon() have not been called
  Status Finish() override;

  // Indicate that the contents of this builder should be abandoned. Stops
  // using the file passed to the constructor after this function returns.
  // If the caller is not going to call Finish(), it must call Abandon()
  // before destroying this builder.
  // REQUIRES: Finish(), Abandon() have not been called
  void Abandon() override;

  // Number of calls to Add() so far.
  uint64_t NumEntries() const override;

  // Size of the file generated so far. If invoked after a successful
  // Finish() call, returns the size of the final generated file.
  uint64_t FileSize() const override;

  bool NeedCompact() const override;

  // Get table properties
  TableProperties GetTableProperties() const override;

  const char* Name() const { return "BlockBasedTable"; }  // Shichao

 private:
  struct Rep;
  Rep* rep_;

  bool ok() const { return status().ok(); }

  // Call block's Finish() method and then write the finalize block contents to
  // file.
  void WriteBlock(BlockBuilder* block, BlockHandle* handle, bool is_data_block);

  // Directly write block content to the file.
  void WriteBlock(const Slice& block_contents, BlockHandle* handle,
                  bool is_data_block);
  void WriteRawBlock(const Slice& data, CompressionType, BlockHandle* handle);

  // Advanced operation: flush any buffered key/value pairs to file.
  // Can be used to ensure that two adjacent entries never live in
  // the same data block. Most clients should not need to use this method.
  // REQUIRES: Finish(), Abandon() have not been called
  void Flush();

  // Some compression libraries fail when the raw size is bigger than int. If
  // uncompressed size is bigger than kCompressionSizeLimit, don't compress it
  const uint64_t kCompressionSizeLimit = std::numeric_limits<int>::max();

  // No copying allowed
  BlockBasedTableBuilder(const BlockBasedTableBuilder&) = delete;
  void operator=(const BlockBasedTableBuilder&) = delete;
};

}  // namespace vidardb
